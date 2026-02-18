#include "pch.h"
#include "TextureFactory.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
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
    
    static void CreatePackageThumbnail(CTexture* Texture)
    {
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        CommandList->Open();
    
        FRHIStagingImageRef StagingImage = GRenderContext->CreateStagingImage(Texture->TextureResource->ImageDescription, ERHIAccess::HostRead);
        CommandList->CopyImage(Texture->TextureResource->RHIImage, FTextureSlice(), StagingImage, FTextureSlice());
    
        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList);
    
        size_t RowPitch = 0;
        void* MappedMemory = GRenderContext->MapStagingTexture(StagingImage, FTextureSlice(), ERHIAccess::HostRead, &RowPitch);
        if (!MappedMemory)
        {
            return;
        }
    
        const uint32 SourceWidth  = Texture->TextureResource->RHIImage->GetDescription().Extent.x;
        const uint32 SourceHeight = Texture->TextureResource->RHIImage->GetDescription().Extent.y;
        
        CPackage* AssetPackage = Texture->GetPackage();
    
        constexpr uint32 ThumbWidth = 256;
        constexpr uint32 ThumbHeight = 256;
        
        AssetPackage->GetPackageThumbnail()->ImageWidth = ThumbWidth;
        AssetPackage->GetPackageThumbnail()->ImageHeight = ThumbHeight;

        constexpr size_t BytesPerPixel = 4;
        constexpr size_t TotalBytes = ThumbWidth * ThumbHeight * BytesPerPixel;
        
        AssetPackage->GetPackageThumbnail()->ImageData.resize(TotalBytes);
        
        const uint8* SourceData = static_cast<const uint8*>(MappedMemory);
        uint8* DestData = AssetPackage->GetPackageThumbnail()->ImageData.data();
        
        const float ScaleX = static_cast<float>(SourceWidth) / ThumbWidth;
        const float ScaleY = static_cast<float>(SourceHeight) / ThumbHeight;
        
        for (uint32 DestY = 0; DestY < ThumbHeight; ++DestY)
        {
            for (uint32 DestX = 0; DestX < ThumbWidth; ++DestX)
            {
                const float SrcX = DestX * ScaleX;
                const float SrcY = DestY * ScaleY;

                const uint32 X0 = static_cast<uint32>(SrcX);
                const uint32 Y0 = static_cast<uint32>(SrcY);
                const uint32 X1 = Math::Min(X0 + 1, SourceWidth - 1);
                const uint32 Y1 = Math::Min(Y0 + 1, SourceHeight - 1);

                const float FracX = SrcX - X0;
                const float FracY = SrcY - Y0;

                const uint8* P00 = SourceData + (Y0 * RowPitch) + (X0 * BytesPerPixel);
                const uint8* P10 = SourceData + (Y0 * RowPitch) + (X1 * BytesPerPixel);
                const uint8* P01 = SourceData + (Y1 * RowPitch) + (X0 * BytesPerPixel);
                const uint8* P11 = SourceData + (Y1 * RowPitch) + (X1 * BytesPerPixel);

                const uint32 FlippedDestY = ThumbHeight - 1 - DestY;
                uint8* DestPixel = DestData + (FlippedDestY * ThumbWidth + DestX) * BytesPerPixel;

                for (uint32 Channel = 0; Channel < BytesPerPixel; ++Channel)
                {
                    const float Top     = Math::Lerp(static_cast<float>(P00[Channel]), static_cast<float>(P10[Channel]), FracX);
                    const float Bottom  = Math::Lerp(static_cast<float>(P01[Channel]), static_cast<float>(P11[Channel]), FracX);
                    const float Result  = Math::Lerp(Top, Bottom, FracY);
    
                    DestPixel[Channel] = static_cast<uint8>(std::lround(Result));
                }
            }
        }
        
        GRenderContext->UnMapStagingTexture(StagingImage);
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
        
        TVector<uint8> Pixels = Result.Pixels;
        
        FRHIImageDesc ImageDescription;
        ImageDescription.Format                         = Result.Format;
        ImageDescription.Extent                         = Result.Dimensions;
        ImageDescription.Flags                          .SetMultipleFlags(EImageCreateFlags::ShaderResource, EImageCreateFlags::Storage);
        ImageDescription.NumMips                        = (uint8)RenderUtils::CalculateMipCount(ImageDescription.Extent.x, ImageDescription.Extent.y);
        ImageDescription.InitialState                   = EResourceStates::ShaderResource;
        ImageDescription.bKeepInitialState              = true;
        NewTexture->TextureResource->ImageDescription   = ImageDescription;

        NewTexture->TextureResource->Mips.resize(ImageDescription.NumMips);

        FRHIImageRef RHIImage = GRenderContext->CreateImage(ImageDescription);
        NewTexture->TextureResource->RHIImage = RHIImage;

        uint32 BytesPerBlock = RHI::Format::BytesPerBlock(ImageDescription.Format);
        const uint32 BaseWidth = ImageDescription.Extent.x;
        const uint32 BaseRowPitch = BaseWidth * BytesPerBlock;

        FRHIStagingImageRef StagingImage = GRenderContext->CreateStagingImage(RHIImage->GetDescription(), ERHIAccess::HostRead);

        // Generate the base mip.
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Compute());
        CommandList->Open();
        
        CommandList->WriteImage(RHIImage, 0, 0, Pixels.data(), BaseRowPitch, 1);
        CommandList->CopyImage(RHIImage, FTextureSlice(), StagingImage, FTextureSlice());
        
        for (uint8 i = 1; i < ImageDescription.NumMips; ++i)
        {
            LUMINA_PROFILE_SECTION_COLORED("Process Mip", tracy::Color::Yellow4);

            FRHIComputeShaderRef ComputeShader = FShaderLibrary::GetComputeShader("GenMipmaps.comp");
        
            FBindingSetDesc SetDesc;
            SetDesc.AddItem(FBindingSetItem::TextureSRV(0, RHIImage, nullptr, RHIImage->GetFormat(), FTextureSubresourceSet(i - 1, 1, 0, 1)));
            SetDesc.AddItem(FBindingSetItem::TextureUAV(1, RHIImage, RHIImage->GetFormat(), FTextureSubresourceSet(i, 1, 0, 1)));

            FRHIBindingSetRef Set;
            FRHIBindingLayoutRef Layout;
            TBitFlags<ERHIShaderType> Visibility;
            Visibility.SetFlag(ERHIShaderType::Compute);
            GRenderContext->CreateBindingSetAndLayout(Visibility, 0, SetDesc, Layout, Set);
        
            FComputePipelineDesc PipelineDesc;
            PipelineDesc.AddBindingLayout(Layout);
            PipelineDesc.CS = ComputeShader;
            PipelineDesc.DebugName = "Gen Mip Maps";
        
            FRHIComputePipelineRef Pipeline = GRenderContext->CreateComputePipeline(PipelineDesc);
        
            FComputeState State;
            State.AddBindingSet(Set);
            State.SetPipeline(Pipeline);
        
            CommandList->SetComputeState(State);
            
            uint32 LevelWidth  = std::max(RenderUtils::GetMipDim(RHIImage->GetSizeX(), i), 1u);
            uint32 LevelHeight = std::max(RenderUtils::GetMipDim(RHIImage->GetSizeY(), i), 1u);
            
            glm::vec2 TexelSize = glm::vec2(1.0f / float(LevelWidth), 1.0f / float(LevelHeight));
        
            uint32_t GroupsX = RenderUtils::GetGroupCount(LevelWidth, 32);
            uint32_t GroupsY = RenderUtils::GetGroupCount(LevelHeight, 32);
            uint32_t GroupsZ = 1;
        
            CommandList->SetPushConstants(&TexelSize, sizeof(glm::vec2));
            CommandList->Dispatch(GroupsX, GroupsY, GroupsZ);

            CommandList->CopyImage(RHIImage, FTextureSlice().SetMipLevel(i), StagingImage, FTextureSlice().SetMipLevel(i));
        }
        
        CommandList->Close();
        GRenderContext->ExecuteCommandList(CommandList, ECommandQueue::Compute);
        
        for (uint8 i = 0; i < ImageDescription.NumMips; ++i)
        {
            FTextureResource::FMip& Mip = NewTexture->TextureResource->Mips[i];
            
            size_t RowPitch = 0;
            void* MappedMemory = GRenderContext->MapStagingTexture(StagingImage, FTextureSlice().SetMipLevel(i), ERHIAccess::HostRead, &RowPitch);
            if (!MappedMemory)
            {
                continue;
            }

            const uint32 Width  = RenderUtils::GetMipDim(RHIImage->GetDescription().Extent.x, i);
            const uint32 Height = RenderUtils::GetMipDim(RHIImage->GetDescription().Extent.y, i);

            Mip.Width       = Width;
            Mip.Height      = Height;
            Mip.RowPitch    = RowPitch;
            Mip.Depth       = 1;
            Mip.SlicePitch  = 1;
            

            Mip.Pixels.resize(RowPitch * Height);
            uint8* Dst = Mip.Pixels.data();
            uint8* Src = (uint8*)MappedMemory;

            for (uint32 Y = 0; Y < Height; ++Y)
            {
                Memory::Memcpy(Dst + Y * RowPitch, Src + Y * RowPitch, RowPitch);
            }
        }

        GRenderContext->UnMapStagingTexture(StagingImage);
        
        CreatePackageThumbnail(NewTexture);

        CPackage* NewPackage = NewTexture->GetPackage();
        CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
        FAssetRegistry::Get().AssetCreated(NewTexture);
        
        NewTexture->ConditionalBeginDestroy();
    }
    
}
