#include "pch.h"
#include "RendererUtils.h"
#include "RenderContext.h"
#include "RenderResource.h"
#include "RHIGlobals.h"
#include "Tools/Import/ImportHelpers.h"

namespace Lumina::RenderUtils
{
    bool ResizeBufferIfNeeded(FRHIBufferRef& Buffer, uint32 DesiredSize, int GrowthFactor)
    {
        if (Buffer->GetSize() < DesiredSize)
        {
            FRHIBufferDesc Desc = Buffer->GetDescription();
            Desc.Size = DesiredSize * GrowthFactor;
            Buffer = GRenderContext->CreateBuffer(Desc);
            return true;
        }
        
        return false;
    }

    FRHIImageRef CreateImageFromPixels(TSpan<uint8> PixelData, bool bFlipVertically, glm::uvec2 Size)
    {
        TOptional<Import::Textures::FTextureImportResult> Result = Import::Textures::ImportTexture(PixelData, bFlipVertically, Size);
        if (!Result.has_value())
        {
            return nullptr;
        }
        
        FRHIImageDesc ImageDescription;
        ImageDescription.Format = Result->Format;
        ImageDescription.Extent = Result->Dimensions;
        ImageDescription.Flags.SetFlag(EImageCreateFlags::ShaderResource);
        ImageDescription.NumMips = 1;
        ImageDescription.InitialState = EResourceStates::ShaderResource;
        ImageDescription.bKeepInitialState = true;
        
        FRHIImageRef ReturnImage = GRenderContext->CreateImage(ImageDescription);

        const uint32 Width = ImageDescription.Extent.x;
        const uint32 RowPitch = Width * RHI::Format::BytesPerBlock(Result->Format);

        FRHICommandListRef TransferCommandList = GRenderContext->CreateCommandList(FCommandListInfo::Transfer());
        TransferCommandList->Open();
        TransferCommandList->WriteImage(ReturnImage, 0, 0, Result->Pixels.data(), RowPitch, 0);
        TransferCommandList->Close();
        GRenderContext->ExecuteCommandList(TransferCommandList, ECommandQueue::Transfer);

        return ReturnImage;
    }
}
