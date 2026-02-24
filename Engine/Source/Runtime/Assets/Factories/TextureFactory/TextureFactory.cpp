#include "pch.h"
#include "TextureFactory.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "encoder/basisu_comp.h"
#include "encoder/basisu_enc.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RendererUtils.h"
#include "Renderer/RenderTypes.h"
#include "Renderer/RHIGlobals.h"
#include "Tools/Import/ImportHelpers.h"

namespace Lumina
{
    CObject* CTextureFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CTexture>(Package, Name);
    }
    
    static void CreatePackageThumbnail(CTexture* Texture, const uint8* RawPixels, uint32 SourceWidth, uint32 SourceHeight)
    {
        CPackage* AssetPackage = Texture->GetPackage();
    
        constexpr uint32 ThumbWidth    = 256;
        constexpr uint32 ThumbHeight   = 256;
        constexpr uint32 BytesPerPixel = 4;
    
        AssetPackage->GetPackageThumbnail()->ImageWidth  = ThumbWidth;
        AssetPackage->GetPackageThumbnail()->ImageHeight = ThumbHeight;
        AssetPackage->GetPackageThumbnail()->ImageData.resize(ThumbWidth * ThumbHeight * BytesPerPixel);
    
        const uint32 SourceRowPitch = SourceWidth * BytesPerPixel;
        const uint8* SourceData     = RawPixels;
        uint8*       DestData       = AssetPackage->GetPackageThumbnail()->ImageData.data();
    
        const float ScaleX = static_cast<float>(SourceWidth)  / ThumbWidth;
        const float ScaleY = static_cast<float>(SourceHeight) / ThumbHeight;
    
        for (uint32 DestY = 0; DestY < ThumbHeight; ++DestY)
        {
            for (uint32 DestX = 0; DestX < ThumbWidth; ++DestX)
            {
                const float SrcX = DestX * ScaleX;
                const float SrcY = DestY * ScaleY;
    
                const uint32 X0 = static_cast<uint32>(SrcX);
                const uint32 Y0 = static_cast<uint32>(SrcY);
                const uint32 X1 = Math::Min(X0 + 1, SourceWidth  - 1);
                const uint32 Y1 = Math::Min(Y0 + 1, SourceHeight - 1);
    
                const float FracX = SrcX - X0;
                const float FracY = SrcY - Y0;
    
                const uint8* P00 = SourceData + (Y0 * SourceRowPitch) + (X0 * BytesPerPixel);
                const uint8* P10 = SourceData + (Y0 * SourceRowPitch) + (X1 * BytesPerPixel);
                const uint8* P01 = SourceData + (Y1 * SourceRowPitch) + (X0 * BytesPerPixel);
                const uint8* P11 = SourceData + (Y1 * SourceRowPitch) + (X1 * BytesPerPixel);
    
                const uint32 FlippedDestY = ThumbHeight - 1 - DestY;
                uint8* DestPixel = DestData + (FlippedDestY * ThumbWidth + DestX) * BytesPerPixel;
    
                for (uint32 Channel = 0; Channel < BytesPerPixel; ++Channel)
                {
                    const float Top    = Math::Lerp(static_cast<float>(P00[Channel]), static_cast<float>(P10[Channel]), FracX);
                    const float Bottom = Math::Lerp(static_cast<float>(P01[Channel]), static_cast<float>(P11[Channel]), FracX);
                    DestPixel[Channel] = static_cast<uint8>(std::lround(Math::Lerp(Top, Bottom, FracY)));
                }
            }
        }
    }
    
    void CTextureFactory::TryImport(const FFixedString& RawPath, const FFixedString& DestinationPath, const Import::FImportSettings* Settings)
    {
        CTexture* NewTexture = TryCreateNew<CTexture>(DestinationPath);
        NewTexture->SetFlag(OF_NeedsPostLoad);

        NewTexture->TextureResource = MakeUnique<FTextureResource>();
        
        TOptional<Import::Textures::FTextureImportResult> MaybeResult;
        
        if (Settings)
        {
            const auto& ImageSettings = Settings->As<Import::Mesh::FMeshImportImage>();
            if (ImageSettings.IsBytes())
            {
                MaybeResult = Import::Textures::ImportTexture(ImageSettings.Bytes, false);
            }
        }
        else
        {
            MaybeResult = Import::Textures::ImportTexture(RawPath, false);
        }
        
        
        
        if (!MaybeResult.has_value())
        {
            NewTexture->ForceDestroyNow();
            return;
        }

        const Import::Textures::FTextureImportResult& Result = MaybeResult.value();
        CreatePackageThumbnail(NewTexture, Result.Pixels.data(), Result.Dimensions.x, Result.Dimensions.y);
        
        TVector<uint8> Pixels = Move(Result.Pixels);
        
        basisu::job_pool JobPool(Threading::GetNumThreads() - 1);
        
        basisu::basis_compressor_params Params;
        Params.m_pJob_pool = &JobPool;
        
        basisu::image BasisImage(Result.Dimensions.x, Result.Dimensions.y);
        Params.m_source_images.resize(1);
        Params.m_source_images[0].init(Pixels.data(), Result.Dimensions.x, Result.Dimensions.y, 4);
        
        Params.m_uastc                      = true;
        Params.m_print_stats                = false;
        Params.m_mip_gen                    = true;
        Params.m_mip_fast                   = true;
        Params.m_multithreading             = true;
        Params.m_create_ktx2_file           = false;
        Params.m_quality_level              = 128;
        Params.m_pack_uastc_ldr_4x4_flags   = basisu::cPackUASTCLevelFastest;
        
        
        basisu::basis_compressor Compressor;
        if (!Compressor.init(Params))
        {
            NewTexture->ForceDestroyNow();
            return;
        }
        
        if (Compressor.process() != basisu::basis_compressor::cECSuccess)
        {
            NewTexture->ForceDestroyNow();
            return;
        }
        
        const basisu::uint8_vec& BasisData = Compressor.get_output_basis_file();
        basist::basisu_transcoder  Transcoder;
        if (!Transcoder.start_transcoding(BasisData.data(), BasisData.size()))
        {
            NewTexture->ForceDestroyNow();
            return;
        }
        
        basist::basisu_file_info FileInfo;
        Transcoder.get_file_info(BasisData.data(), BasisData.size(), FileInfo);
        const uint32 NumMips = FileInfo.m_image_mipmap_levels[0];
        
        basist::basisu_image_info ImageInfo;
        Transcoder.get_image_info(BasisData.data(), BasisData.size(), ImageInfo, 0);
        const uint32 Width  = ImageInfo.m_width;
        const uint32 Height = ImageInfo.m_height;
        
        FRHIImageDesc ImageDescription;
        ImageDescription.Format                         = EFormat::BC7_UNORM;
        ImageDescription.Extent                         = glm::uvec2(Width, Height);
        ImageDescription.Flags                          .SetMultipleFlags(EImageCreateFlags::ShaderResource);
        ImageDescription.NumMips                        = static_cast<uint8>(NumMips);
        ImageDescription.InitialState                   = EResourceStates::ShaderResource;
        ImageDescription.bKeepInitialState              = true;
        NewTexture->TextureResource->ImageDescription   = ImageDescription;

        NewTexture->TextureResource->Mips.resize(NumMips);

        FRHIImageRef RHIImage = GRenderContext->CreateImage(ImageDescription);
        NewTexture->TextureResource->RHIImage = RHIImage;

        uint32 BytesPerBlock = RHI::Format::BytesPerBlock(ImageDescription.Format);
        
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Compute());
        CommandList->Open();
        
        for (uint32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
        {
            basist::basisu_image_level_info LevelInfo;
            
            if (!Transcoder.get_image_level_info(BasisData.data(), BasisData.size(), LevelInfo, 0, MipIndex))
            {
                continue;
            }

            const uint32 BlocksX     = LevelInfo.m_num_blocks_x;
            const uint32 BlocksY     = LevelInfo.m_num_blocks_y;
            const uint32 TotalBlocks = LevelInfo.m_total_blocks;
            const uint32 RowPitch    = BlocksX * BytesPerBlock;
            const uint32 DepthPitch  = RowPitch * BlocksY;

            TVector<uint8> TranscodedData(TotalBlocks * BytesPerBlock);
            
            if (!Transcoder.transcode_image_level(
                    BasisData.data(), BasisData.size(),
                    0,
                    MipIndex,
                    TranscodedData.data(), TotalBlocks,
                    basist::transcoder_texture_format::cTFBC7_RGBA))
            {
                continue;
            }
            
            CommandList->WriteImage(RHIImage, 0, MipIndex, TranscodedData.data(), RowPitch, 1);
            
            FTextureResource::FMip& Mip = NewTexture->TextureResource->Mips[MipIndex];
            Mip.Width      = LevelInfo.m_width;
            Mip.Height     = LevelInfo.m_height;
            Mip.RowPitch   = RowPitch;
            Mip.Depth      = 1;
            Mip.SlicePitch = DepthPitch;
            Mip.Pixels     = Move(TranscodedData);
        }
        
        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList, ECommandQueue::Compute);
        
        CPackage* NewPackage = NewTexture->GetPackage();
        CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
        FAssetRegistry::Get().AssetCreated(NewTexture);
        
        NewTexture->ConditionalBeginDestroy();
    }
    
}
