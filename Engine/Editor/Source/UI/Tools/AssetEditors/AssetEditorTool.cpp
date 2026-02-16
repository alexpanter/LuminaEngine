#include "AssetEditorTool.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "FileSystem/FileSystem.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "World/Scene/RenderScene/RenderScene.h"

namespace Lumina
{
    void FAssetEditorTool::OnInitialize()
    {
    }

    void FAssetEditorTool::Deinitialize(const FUpdateContext& UpdateContext)
    {
        FEditorTool::Deinitialize(UpdateContext);
    }

    FName FAssetEditorTool::GetToolName() const
    {
        return Asset->GetName();
    }

    void FAssetEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        if (!bAssetLoadBroadcasted && Asset != nullptr)
        {
            OnAssetLoadFinished();
            bAssetLoadBroadcasted = true;
        }
    }

    void FAssetEditorTool::OnSave()
    {
        if (ShouldGenerateThumbnailOnSave() && Asset->GetPackage())
        {
            GenerateThumbnail(Asset->GetPackage());
        }
        
        if (CPackage::SavePackage(Asset->GetPackage(), Asset->GetPackage()->GetPackagePath()))
        {
            FAssetRegistry::Get().AssetSaved(Asset);
            ImGuiX::Notifications::NotifySuccess("Successfully saved package: \"{0}\"", Asset->GetName().c_str());
        }
        else
        {
            ImGuiX::Notifications::NotifyError("Failed to save package: \"{0}\"", Asset->GetName().c_str());
        }
    }

    bool FAssetEditorTool::IsAssetEditorTool() const
    {
        return true;
    }
}
