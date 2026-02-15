#include "ContentBrowserEditorTool.h"

#include "EditorToolContext.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/Factories/Factory.h"
#include "Core/Object/Package/Package.h"
#include "EASTL/sort.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Platform/Process/PlatformProcess.h"
#include "Scripting/Lua/Scripting.h"
#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/ThreadedCallback.h"
#include "Tools/Dialogs/Dialogs.h"
#include "Tools/UI/ImGui/ImGuiFonts.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include <string.h>
#include <cstdarg>
#include <filesystem>
#include <format>
#include <iterator>
#include <string>
#include <Assets/AssetRegistry/AssetData.h>
#include <Containers/Array.h>
#include <Containers/Function.h>
#include <Containers/String.h>
#include <Core/LuminaCommonTypes.h>
#include <Core/Math/Hash/Hash.h>
#include <Core/Object/Class.h>
#include <Core/Object/Object.h>
#include <Core/Object/ObjectCore.h>
#include <Core/Templates/LuminaTemplate.h>
#include <Events/Event.h>
#include <FileSystem/FileInfo.h>
#include <Memory/SmartPtr.h>
#include <Platform/Filesystem/DirectoryWatcher.h>
#include <Platform/GenericPlatform.h>
#include <Platform/Platform.h>
#include <Tools/UI/ImGui/ImGuiDesignIcons.h>
#include <Tools/UI/ImGui/Widgets/TileViewWidget.h>
#include <Tools/UI/ImGui/Widgets/TreeListView.h>
#include <EASTL/any.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "Thumbnails/ThumbnailManager.h"
#include <LuminaEditor.h>

namespace Lumina
{

    template<size_t BufferSize = 42>
    class FRenameModalState
    {
    public:
        
        void Initialize(FStringView CurrentName)
        {
            Buffer.assign(CurrentName.begin(), CurrentName.end());
        }

        
        // ReSharper disable once CppMemberFunctionMayBeStatic
        NODISCARD constexpr size_t Capacity() const { return BufferSize; }
        FORCEINLINE NODISCARD char* CStr() { return Buffer.data(); }
        FORCEINLINE NODISCARD bool IsValid() const { return !Buffer.empty(); }
        
    private:
        
        TFixedString<BufferSize> Buffer;
    };

    bool FContentBrowserEditorTool::OnEvent(FEvent& Event)
    {
        if (Event.IsA<FFileDropEvent>())
        {
            FFileDropEvent& FileEvent = Event.As<FFileDropEvent>();

            ImVec2 DropCursor = ImVec2(FileEvent.GetMouseX(), FileEvent.GetMouseY());

            for (const FFixedString& Path : FileEvent.GetPaths())
            {
                ActionRegistry.EnqueueAction<FPendingOSDrop>(FPendingOSDrop{ Path, DropCursor });
            }

            return true;
        }

        return false;
    }

    void FContentBrowserEditorTool::RefreshContentBrowser()
    {
        ContentBrowserTileView.MarkTreeDirty();
        DirectoryListView.MarkTreeDirty();
    }

    void FContentBrowserEditorTool::OnInitialize()
    {
        (void)FAssetRegistry::Get().GetOnAssetRegistryUpdated().AddMember(this, &FContentBrowserEditorTool::RefreshContentBrowser);
        (void)GEditorEngine->GetProjectLoadedDelegate().AddMember(this, &FContentBrowserEditorTool::OnProjectLoaded);

        if (GEditorEngine->HasLoadedProject())
        {
            SelectedPath = GEditorEngine->GetProjectContentDirectory();
        }

        const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
        for (CFactory* Factory : Factories)
        {
            if (CClass* AssetClass = Factory->GetAssetClass())
            {
                FilterState.emplace(AssetClass->GetName().c_str(), true);
            }
        }
        
        CreateToolWindow("Content", [&] (bool bIsFocused)
        {
            float Left = 200.0f;
            float Right = ImGui::GetContentRegionAvail().x - Left;
            
            DrawDirectoryBrowser(bIsFocused, ImVec2(Left, 0));
            
            ImGui::SameLine();

            DrawContentBrowser(bIsFocused, ImVec2(Right, 0));
        });
        
        ContentBrowserTileViewContext.DragDropFunction = [this] (FTileViewItem* DropItem)
        {
            auto* TypedDroppedItem = (FContentBrowserTileViewItem*)DropItem;
            if (!TypedDroppedItem->IsDirectory())
            {
                return;
            }
            
            const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(FContentBrowserTileViewItem::DragDropID, ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (Payload && Payload->IsDelivery())
            {
                uintptr_t ValuePtr = *static_cast<uintptr_t*>(Payload->Data);
                auto* SourceItem = reinterpret_cast<FContentBrowserTileViewItem*>(ValuePtr);

                if (SourceItem == TypedDroppedItem)
                {
                    return;
                }

                HandleContentBrowserDragDrop(TypedDroppedItem->GetVirtualPath(), SourceItem->GetVirtualPath());
            }
        };

        ContentBrowserTileViewContext.DrawItemOverrideFunction = [this] (FTileViewItem* Item) -> bool
        {
            FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);
            
            ImVec4 TintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            
            
            ImTextureRef ImTexture;
            if (ContentItem->IsDirectory())
            {
                ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/Folder.png");
                TintColor = ImVec4(1.0f, 0.9f, 0.6f, 1.0f);
            }
            else if (ContentItem->IsAsset())
            {
                if (FPackageThumbnail* MaybeThumbnail = CThumbnailManager::Get().GetThumbnailForPackage(ContentItem->GetVirtualPath()))
                {
                    ImTexture = ImGuiX::ToImTextureRef(MaybeThumbnail->LoadedImage);
                }
                else
                {
                    ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/Asset.png");
                }
            }
            else if (ContentItem->IsLuaScript())
            {
                ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/LuaScript.png");
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.17f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.26f, 0.28f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        
            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            ImVec2 Pos = ImGui::GetCursorScreenPos();
            ImVec2 Size = ImVec2(120.0f, 120.0f);
            
            DrawList->AddRectFilled(
                ImVec2(Pos.x + 3, Pos.y + 3),
                ImVec2(Pos.x + Size.x + 11, Pos.y + Size.y + 11),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.3f)),
                8.0f
            );
            
            ImGui::ImageButton("##", ImTexture, Size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), TintColor);
        
            if (ImGui::IsItemHovered())
            {
                DrawList->AddRect(
                    Pos, 
                    ImVec2(Pos.x + Size.x + 8, Pos.y + Size.y + 8), 
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.6f, 0.9f, 0.7f)), 
                    8.0f, 
                    0, 
                    2.0f
                );
            }
        
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
        
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                return true;
            }
        
            return false;
        };
        
        ContentBrowserTileViewContext.ItemSelectedFunction = [this] (FTileViewItem* Item)
        {
            FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);
            FFixedString Path {ContentItem->GetVirtualPath().data(), ContentItem->GetVirtualPath().size()};
            
            if (ContentItem->IsDirectory())
            {
                SelectedPath = Move(Path);
                RefreshContentBrowser();
            }
            else if (ContentItem->IsAsset())
            {
                if (const FAssetData* Asset = FAssetRegistry::Get().GetAssetByPath(Path))
                {
                    ToolContext->OpenAssetEditor(Asset->AssetGUID);
                }
            }
            else if (ContentItem->IsLuaScript())
            {
                ToolContext->OpenScriptEditor(ContentItem->GetPathSource());
            }
        };
        
        ContentBrowserTileViewContext.DrawItemContextMenuFunction = [this] (const TVector<FTileViewItem*>& Items)
        {
            bool bMultipleItems = Items.size() > 1;
            
            for (FTileViewItem* Item : Items)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);

                if (bMultipleItems)
                {
                    continue;
                }

                if (ContentItem->IsAsset())
                {
                    DrawAssetContextMenu(ContentItem);
                }
                else if (ContentItem->IsLuaScript())
                {
                    DrawLuaScriptContextMenu(ContentItem);
                }
                else if (ContentItem->IsDirectory())
                {
                    DrawDirectoryContextMenu(ContentItem);
                }
            }
        };

        ContentBrowserTileViewContext.RebuildTreeFunction = [this] (FTileViewWidget* Tree)
        {
            
            TVector<VFS::FFileInfo> SortedPaths;
            
            VFS::DirectoryIterator(SelectedPath, [&](const VFS::FFileInfo& FileInfo)
            {
                if (!FileInfo.IsDirectory())
                {
                    if (!FileInfo.IsLAsset() && !FileInfo.IsLua())
                    {
                        return;
                    }
                }

                SortedPaths.emplace_back(FileInfo);
            });
            
            eastl::sort(SortedPaths.begin(), SortedPaths.end(), [&](const VFS::FFileInfo& LHS, const VFS::FFileInfo& RHS)
            {
                if (LHS.IsDirectory() != RHS.IsDirectory())
                {
                    return LHS.IsDirectory();
                }
                
                return LHS.Name < RHS.Name;
            });
            
            for (const VFS::FFileInfo& Info : SortedPaths)
            {
                ContentBrowserTileView.AddItemToTree<FContentBrowserTileViewItem>(nullptr, Info);
            }
        };

        ContentBrowserTileViewContext.KeyPressedFunction = [this] (FTileViewItem& Item, ImGuiKey Key) -> bool
        {
            if (Key == ImGuiKey_F2)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(&Item);
                PushRenameModal(ContentItem);
                return true;
            }

            if (Key == ImGuiKey_Delete)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(&Item);
                OpenDeletionWarningPopup(ContentItem);
                return true;
            }

            return false;
        };
        
        DirectoryContext.ItemContextMenuFunction = [this](FTreeListView& Tree, entt::entity Item)
        {
            
        };

        DirectoryContext.DragDropFunction = [this](FTreeListView& Tree, entt::entity Item)
        {
            FContentBrowserListViewItemData& Data = Tree.Get<FContentBrowserListViewItemData>(Item);
            const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(FContentBrowserTileViewItem::DragDropID, ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (Payload && Payload->IsDelivery())
            {
                uintptr_t ValuePtr = *static_cast<uintptr_t*>(Payload->Data);
                auto* SourceItem = reinterpret_cast<FContentBrowserTileViewItem*>(ValuePtr);
                
                HandleContentBrowserDragDrop(Data.Path, SourceItem->GetVirtualPath());
            }
        };
        
        DirectoryContext.RebuildTreeFunction = [this](FTreeListView& Tree)
        {
            
            TFunction<void(entt::entity, FStringView)> AddChildrenRecursive;
            
            AddChildrenRecursive = [&](entt::entity ParentItem, FStringView CurrentPath)
            {
                VFS::DirectoryIterator(CurrentPath, [&](const VFS::FFileInfo& Info)
                {
                    if (!Info.IsDirectory())
                    {
                        return;
                    }
                    
                    FFixedString DisplayName;
                    DisplayName.append(LE_ICON_FOLDER).append(" ").append(Info.Name.begin(), Info.Name.end());
            
                    entt::entity ItemEntity = Tree.CreateNode(ParentItem, DisplayName, Hash::GetHash64(Info.PathSource));
                    Tree.EmplaceUserData<FContentBrowserListViewItemData>(ItemEntity).Path.assign(Info.VirtualPath.begin(), Info.VirtualPath.end());
                    
                    if (Info.VirtualPath == SelectedPath)
                    {
                        FTreeNodeState& State = Tree.Get<FTreeNodeState>(ItemEntity);
                        State.bSelected = true;
                    }
            
                    AddChildrenRecursive(ItemEntity, Info.VirtualPath); 
                    
                });
            };
            
            
            FFixedString Name;
            Name.assign(LE_ICON_FOLDER).append(" Game");
            entt::entity RootItem = Tree.CreateNode(entt::null, Name);
            Tree.EmplaceUserData<FContentBrowserListViewItemData>(RootItem).Path = "/Game";
            
            AddChildrenRecursive(RootItem, "/Game");
            
            Name.assign(LE_ICON_FOLDER).append(" Engine");
            RootItem = Tree.CreateNode(entt::null, Name);
            Tree.EmplaceUserData<FContentBrowserListViewItemData>(RootItem).Path = "/Engine/Resources/Content";
            
            AddChildrenRecursive(RootItem, "/Engine/Resources/Content");
        };

        DirectoryContext.ItemSelectedFunction = [this] (FTreeListView& Tree, entt::entity Item)
        {
            if (Item == entt::null)
            {
                return;
            }
            
            FContentBrowserListViewItemData& Data = Tree.Get<FContentBrowserListViewItemData>(Item);
            
            SelectedPath = Data.Path;

            RefreshContentBrowser();
        };

        DirectoryContext.KeyPressedFunction = [this] (FTreeListView& Tree, entt::entity Item, ImGuiKey Key) -> bool
        {
            return false;
        };
        
        DirectoryListView.MarkTreeDirty();
        ContentBrowserTileView.MarkTreeDirty();
    }

    void FContentBrowserEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        
    }
    
    void FContentBrowserEditorTool::EndFrame()
    {
        bool bWroteSomething = false;
        
        ActionRegistry.ProcessAllOf<FPendingDestroy>([&] (const FPendingDestroy& Destroy)
        {
            CObject* AliveObject = nullptr;
            if (VFS::HasExtension(Destroy.PendingDestroy, ".lasset"))
            {
                if (const FAssetData* Data = FAssetRegistry::Get().GetAssetByPath(Destroy.PendingDestroy))
                {
                    if (CObject* Object = FindObject<CObject>(Data->AssetGUID))
                    {
                        AliveObject = Object;
                        if (AliveObject->IsA<CWorld>())
                        {
                            ImGuiX::Notifications::NotifyError("Cannot destroy a world that's open {0}", Destroy.PendingDestroy);
                            return;
                        }
                    }
                }
            }
            
            if (VFS::IsDirectory(Destroy.PendingDestroy))
            {
                VFS::RemoveAll(Destroy.PendingDestroy);
                ImGuiX::Notifications::NotifySuccess("Deleted Directory {0}", Destroy.PendingDestroy);
            }
            else if (AliveObject)
            {
                ToolContext->OnDestroyAsset(AliveObject);
                
                if (CPackage::DestroyPackage(Destroy.PendingDestroy))
                {
                    ImGuiX::Notifications::NotifySuccess("Deleted Asset {0}", Destroy.PendingDestroy);
                }
            }

            bWroteSomething = true;
		});
        
        ActionRegistry.ProcessAllOf<FPendingRename>([&](FPendingRename& Rename)
        {
            VFS::Rename(Rename.OldName, Rename.NewName);
            
            FStringView Extension = VFS::Extension(Rename.OldName);
            
            if (Extension == ".lasset")
            {
                CPackage::RenamePackage(Rename.OldName, Rename.NewName);
                FAssetRegistry::Get().AssetRenamed(Rename.OldName, Rename.NewName);
            }
            else if (Extension.empty())
            {
                VFS::RecursiveDirectoryIterator(Rename.NewName, [&](const VFS::FFileInfo& FileInfo)
                {
                    if (FileInfo.IsDirectory())
                    {
                        return;
                    }
                    
                    FFixedString CurrentPath = FileInfo.VirtualPath;
                    FFixedString OldObjectName(Rename.OldName.begin(), Rename.OldName.end());
                    if (Rename.OldName.find(Rename.NewName.c_str()) == FString::npos)
                    {
                        OldObjectName.assign_convert(Rename.OldName) + (CurrentPath.substr(0, Rename.NewName.length()));
                    }
                    
                    CPackage::RenamePackage(OldObjectName, CurrentPath);
                    FAssetRegistry::Get().AssetRenamed(OldObjectName, CurrentPath);
                });
            }
            
            ImGuiX::Notifications::NotifySuccess("Rename Success");
            bWroteSomething = true;
        });


        if (bWroteSomething)
        {
            RefreshContentBrowser();
        }
    }
    
    void FContentBrowserEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGuiID topDockID = 0, bottomLeftDockID = 0, bottomCenterDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Down, 0.5f, &bottomCenterDockID, &topDockID);
        ImGui::DockBuilderSplitNode(bottomCenterDockID, ImGuiDir_Right, 0.66f, &bottomCenterDockID, &bottomLeftDockID);
        ImGui::DockBuilderSplitNode(bottomCenterDockID, ImGuiDir_Right, 0.5f, &bottomRightDockID, &bottomCenterDockID);

        ImGui::DockBuilderDockWindow(GetToolWindowName("Content").c_str(), bottomCenterDockID);
    }

    void FContentBrowserEditorTool::DrawToolMenu(const FUpdateContext& UpdateContext)
    {
        if (ImGui::BeginMenu(LE_ICON_FILTER " Filter"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));

            for (auto& [Name, State] : FilterState)
            {
                if (ImGui::Checkbox(Name.c_str(), &State))
                {
                    RefreshContentBrowser();
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::EndMenu();
        }
    }

    void FContentBrowserEditorTool::HandleContentBrowserDragDrop(FStringView DropPath, FStringView PayloadPath)
    {
        size_t Pos = PayloadPath.find_last_of('/');
        FStringView DirName = (Pos != FString::npos) ? PayloadPath.substr(Pos + 1) : PayloadPath;
        
        FFixedString OldName(PayloadPath.data(), PayloadPath.length());
        FFixedString NewName = Paths::Combine(DropPath, DirName);

        ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ OldName, NewName });
    }

    void FContentBrowserEditorTool::OpenDeletionWarningPopup(const FContentBrowserTileViewItem* Item, const TFunction<void(EYesNo)>& Callback)
    {
        if (VFS::IsEmpty(Item->GetVirtualPath()))
        {
            if (Callback)
            {
                Callback(EYesNo::Yes);
            }
            ActionRegistry.EnqueueAction<FPendingDestroy>(FPendingDestroy{ FFixedString(Item->GetVirtualPath().data(), Item->GetVirtualPath().size()) });
        }
        else if (Dialogs::Confirmation("Confirm Deletion", "Are you sure you want to delete \"{0}\"?\n""\nThis action cannot be undone.", Item->GetName()))
        {
            if (Callback)
            {
                Callback(EYesNo::Yes);
            }
            ActionRegistry.EnqueueAction<FPendingDestroy>(FPendingDestroy{ FFixedString(Item->GetVirtualPath().data(), Item->GetVirtualPath().size()) });
        }

        if (Callback)
        {
            Callback(EYesNo::No);
        }
    }

    void FContentBrowserEditorTool::OnProjectLoaded()
    {
        FFixedString ScriptPath = GEditorEngine->GetProjectScriptDirectory();
        
        Watcher.Stop();
        Watcher.Watch(ScriptPath, [&](const FFileEvent& Event)
        {
            if (!VFS::HasExtension(Event.Path, ".lua"))
            {
                return;
            }
            
            FStringView Prefix = "/Game/Scripts";
            size_t Pos = Event.Path.find(Prefix.data(), 0, Prefix.size());
            if (Pos == FString::npos)
            {
                return;
            }
            
            FFixedString RelativePath;
            RelativePath.append_convert(Prefix.data(), Prefix.size()).append_convert(Event.Path.substr(Pos + Prefix.size()));
            
            switch (Event.Action)
            {
            case EFileAction::Added:
                {
                    Scripting::FScriptingContext::Get().ScriptCreated(RelativePath);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Modified:
                {
                    Scripting::FScriptingContext::Get().ScriptReloaded(RelativePath);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Removed:
                {
                    Scripting::FScriptingContext::Get().ScriptDeleted(RelativePath);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Renamed:
                {
                    FFixedString RelativeOldPath;
                    size_t OldPos = Event.OldPath.find(Prefix.data(), 0, Prefix.size());
                    if (OldPos == FString::npos)
                    {
                        return;
                    }
                    
                    RelativePath.append_convert(Prefix.data(), Prefix.size()).append_convert(Event.OldPath.substr(OldPos + Prefix.size()));
                    Scripting::FScriptingContext::Get().ScriptRenamed(RelativePath, RelativeOldPath);
                    RefreshContentBrowser();
                }
                break;
            }
        });
    }

    void FContentBrowserEditorTool::TryImport(const FFixedString& Path)
    {
        const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
        for (CFactory* Factory : Factories)
        {
            if (!Factory->CanImport())
            {
                continue;
            }
        
            FStringView Ext = VFS::Extension(Path);
            if (!Factory->IsExtensionSupported(Ext))
            {
                continue;
            }
            
            
            FStringView FileName = VFS::FileName(Path);
            FFixedString DestinationPath = Paths::Combine(SelectedPath, FileName);
            DestinationPath = VFS::MakeUniqueFilePath(DestinationPath);
            
            if (Factory->HasImportDialogue())
            {
                struct FModelState
                {
                    eastl::any ImportSettings;
                    bool bShouldClose = false;
                } State;
                
                auto SharedState = MakeShared<FModelState>();
                
                ToolContext->PushModal("Import", {700, 800}, [this, Factory, Path = Move(Path), DestinationPath = Move(DestinationPath), SharedState] () mutable
                {
                    if (Factory->DrawImportDialogue(Path, DestinationPath, SharedState->ImportSettings, SharedState->bShouldClose))
                    {
                        Task::AsyncTask(1, 1, [Factory, Path, DestinationPath, ImportSettings = Move(SharedState->ImportSettings)](uint32, uint32, uint32)
                        {
                            Factory->Import(Path, DestinationPath, ImportSettings);
                            
                            MainThread::Enqueue([Path = Move(Path)] ()
                            {
                                ImGuiX::Notifications::NotifySuccess("Successfully Imported: \"{0}\"", Path);
                            });
                        });
                    }
                    
                    return SharedState->bShouldClose;
                });
            }
            else
            {
                Task::AsyncTask(1, 1, [this, Factory, Path = Move(Path), PathString = Move(DestinationPath)] (uint32, uint32, uint32)
                {
                    Factory->Import(Path, PathString);
                    
                    MainThread::Enqueue([Path = Move(Path)] ()
                    {
                        ImGuiX::Notifications::NotifySuccess("Successfully Imported: \"{0}\"", Path);
                    });            
                });
            }
        }
    }

    void FContentBrowserEditorTool::PushRenameModal(FContentBrowserTileViewItem* ContentItem)
    {
        ToolContext->PushModal("Rename", ImVec2(480.0f, 300.0f), [this, ContentItem, RenameState = MakeUnique<FRenameModalState<>>()]
        {
            RenameState->Initialize(ContentItem->GetName());
            
            const ImGuiStyle& style = ImGui::GetStyle();
            const float ContentWidth = ImGui::GetContentRegionAvail().x;
            
            ImGuiX::Font::PushFont(ImGuiX::Font::EFont::MediumBold);
            ImGuiX::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), LE_ICON_ARCHIVE_EDIT " Rename {0}", ContentItem->IsDirectory() ? "Folder" : "Asset");
            ImGuiX::Font::PopFont();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGuiX::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "Current name:");
            
            ImGui::SameLine();
            
            ImGuiX::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1.0f), "{0}", ContentItem->GetName());
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGuiX::TextColoredUnformatted(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "New name:");
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            ImGui::SetNextItemWidth(-1);
            
            bool bSubmitted = ImGui::InputText("##RenameInput", RenameState->CStr(), RenameState->Capacity(), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
            
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            bool bIsValid = RenameState->IsValid();
            bool bNameUnchanged = strcmp(RenameState->CStr(), ContentItem->GetName().data()) == 0;
            FString ValidationMessage;
            bool bHasError = false;
            
            if (RenameState->IsValid())
            {
                if (bNameUnchanged)
                {
                    ValidationMessage = "Name unchanged - please enter a different name";
                    bHasError = true;
                    bIsValid = false;
                }
                else
                {
                    FStringView Extension = ContentItem->GetExtension();
                    FStringView PathNoExt = VFS::RemoveExtension(ContentItem->GetVirtualPath());
                    FFixedString TestPath = Paths::Combine(PathNoExt, RenameState->CStr());
                    TestPath.append_convert(Extension.data(), Extension.length());
                    
                    if (VFS::Exists(TestPath))
                    {
                        ValidationMessage = std::format("Path already exists: {}", TestPath.c_str()).c_str();
                        bHasError = true;
                        bIsValid = false;
                    }
                }
            }
            
            if (bHasError && !ValidationMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.1f, 0.1f, 0.3f));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 0.4f));
                
                ImGui::BeginChild("##ValidationError", ImVec2(-1, 45.0f), true);
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::Text(LE_ICON_ALERT_OCTAGON);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", ValidationMessage.c_str());
                ImGui::PopStyleColor();
                
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);
                
                ImGui::Spacing();
            }
            
            if (bSubmitted && bIsValid)
            {
                FStringView PathNoExt = VFS::RemoveExtension(ContentItem->GetVirtualPath());
                FFixedString TestPath = Paths::Combine(VFS::Parent(PathNoExt), RenameState->CStr());
                TestPath.append_convert(ContentItem->GetExtension());
                
                ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ FFixedString(ContentItem->GetVirtualPath().data(), ContentItem->GetVirtualPath().length()), TestPath });
                return true;
            }
            
            ImGui::Spacing();
            
            ImGui::Separator();
            ImGui::Spacing();

            constexpr float ButtonHeight = 32.0f;
            const float ButtonWidth = (ContentWidth - style.ItemSpacing.x) * 0.5f;
            
            if (!bIsValid)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.85f, 1.0f));
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            if (ImGui::Button(LE_ICON_CHECK " Rename", ImVec2(ButtonWidth, ButtonHeight)))
            {
                if (bIsValid)
                {
                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();

                    FStringView PathNoExt = VFS::RemoveExtension(ContentItem->GetVirtualPath());
                    FFixedString TestPath = Paths::Combine(VFS::Parent(PathNoExt), RenameState->CStr());
                    TestPath.append_convert(ContentItem->GetExtension());
                
                    ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ FFixedString(ContentItem->GetVirtualPath().data(), ContentItem->GetVirtualPath().length()), TestPath });
                    return true;
                }
            }
            
            ImGui::PopStyleVar();
            
            if (!bIsValid)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::PopStyleColor(3);
            }
            
            ImGui::SameLine();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
            
            if (ImGui::Button(LE_ICON_CANCEL " Cancel", ImVec2(ButtonWidth, ButtonHeight)))
            {
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                
                return true;
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                return true;
            }
            
            return false;
        });
    }

    void FContentBrowserEditorTool::DrawDirectoryBrowser(bool bIsFocused, ImVec2 Size)
    {
        ImGui::BeginChild("Directories", Size);

        DirectoryListView.Draw(DirectoryContext);
        
        ImGui::EndChild();
    }

    void FContentBrowserEditorTool::DrawContentBrowser(bool bIsFocused, ImVec2 Size)
    {
        constexpr float Padding = 10.0f;

        ImVec2 AdjustedSize = ImVec2(Size.x - 2 * Padding, 0.0f);

        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(Padding, Padding));

        ImGui::BeginChild("Content", AdjustedSize, true, ImGuiWindowFlags_None);
        
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("ContentContextMenu");
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 100.0f), ImVec2(0.0f, 0.0f));
        
        if (ImGui::BeginPopup("ContentContextMenu"))
        {
            const char* FolderIcon = LE_ICON_FOLDER;
            FString MenuItemName = FString(FolderIcon) + " " + "New Folder";
            if (ImGui::MenuItem(MenuItemName.c_str()))
            {
                FString PathString = FString(SelectedPath + "/NewFolder");
                VFS::CreateDir(PathString);
                RefreshContentBrowser();
            }

            if (SelectedPath.find("/Game/Scripts") != FString::npos)
            {
                DrawScriptsDirectoryContextMenu();
            }
            else
            {
                DrawContentDirectoryContextMenu();
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::BeginHorizontal("Breadcrumbs");

        auto GameDirPos = SelectedPath.find("Game");
        if (GameDirPos != std::string::npos)
        {
            FFixedString BasePathStr = SelectedPath.substr(0, GameDirPos);
            std::filesystem::path BasePath(BasePathStr.c_str());
            std::filesystem::path RelativePath = std::filesystem::path(SelectedPath.c_str()).lexically_relative(BasePath);
    
            std::filesystem::path BuildingPath = BasePath;
    
            for (auto it = RelativePath.begin(); it != RelativePath.end(); ++it)
            {
                BuildingPath /= *it;
        
                ImGui::PushID(static_cast<int>(std::distance(RelativePath.begin(), it)));
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
            
                    if (ImGui::Button(it->string().c_str()))
                    {
                        SelectedPath = BuildingPath.generic_string().c_str();
                        ContentBrowserTileView.MarkTreeDirty();
                    }
            
                    ImGui::PopStyleVar(2);
                }
                ImGui::PopID();
        
                if (std::next(it) != RelativePath.end())
                {
                    ImGui::TextUnformatted(LE_ICON_ARROW_RIGHT);
                }
            }
        }

        ImGui::EndHorizontal();

        ImGui::Separator();
        
        ContentBrowserTileView.Draw(ContentBrowserTileViewContext);

        ImVec2 ChildMin = ImGui::GetWindowPos();
        ImVec2 ChildMax = ImVec2(ChildMin.x + ImGui::GetWindowWidth(), ChildMin.y + ImGui::GetWindowHeight());
        
        ImRect Rect(ChildMin, ChildMax);

        ActionRegistry.ProcessAllOf<FPendingOSDrop>([&](const FPendingOSDrop& Drop)
        {
            if (Rect.Contains(Drop.MousePos))
            {
                TryImport(Drop.Path);
            }
		});
        
        ImGui::EndChild();
    
    }

    void FContentBrowserEditorTool::DrawDirectoryContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        ImGui::Separator();
        
        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FStringView VirtualPath = ContentItem->GetVirtualPath();
            FStringView Parent = VFS::Parent(VirtualPath);
            Platform::LaunchURL(StringUtils::ToWideString(Parent).c_str());
        }

        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetVirtualPath().data());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();

        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }
    }

    void FContentBrowserEditorTool::DrawLuaScriptContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        ImGui::Separator();

        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FStringView Parent = Paths::Parent(ContentItem->GetVirtualPath());
            Platform::LaunchURL(StringUtils::ToWideString(Parent).c_str());
        }
        
        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetVirtualPath().data());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }
        
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();

        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }
    }

    void FContentBrowserEditorTool::DrawAssetContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
                    
        ImGui::Separator();
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FStringView Parent = Paths::Parent(ContentItem->GetVirtualPath());
            Platform::LaunchURL(StringUtils::ToWideString(Parent).c_str());
        }
        
        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetVirtualPath().data());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }
        
        ImGui::Separator();
        
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();
        
        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }

        if (ContentItem->IsAsset())
        {
            if (ImGui::MenuItem(LE_ICON_FOLDER_OPEN " Open", "Double-Click"))
            {
                FFixedString Path(ContentItem->GetVirtualPath().data(), ContentItem->GetVirtualPath().size());
                if (const FAssetData* Data = FAssetRegistry::Get().GetAssetByPath(Path))
                {
                    ToolContext->OpenAssetEditor(Data->AssetGUID);
                }
            }
            ImGui::Separator();
        }
    }


    void FContentBrowserEditorTool::DrawScriptsDirectoryContextMenu()
    {
        if (ImGui::MenuItem(LE_ICON_OPEN_IN_NEW " New Script"))
        {
            FFixedString NewScriptPath = SelectedPath + "/" + "NewScript.lua";
            VFS::WriteFile(NewScriptPath, "-- New Lua Script");
        }
    }

    void FContentBrowserEditorTool::DrawContentDirectoryContextMenu()
    {
        const char* ImportIcon = LE_ICON_FILE_IMPORT;
        FString MenuItemName = FString(ImportIcon) + " " + "Import Asset";
        if (ImGui::MenuItem(MenuItemName.c_str()))
        {
            FFixedString SelectedFile;
            const char* Filter = "Supported Assets (*.png;*.jpg;*.fbx;*.gltf;*.glb;*.obj)\0*.png;*.jpg;*.fbx;*.gltf;*.glb;*.obj\0All Files (*.*)\0*.*\0";
            if (Platform::OpenFileDialogue(SelectedFile, "Import Asset", Filter))
            {
                TryImport(SelectedFile);
            }
        }

        const char* FileIcon = LE_ICON_PLUS;
        const char* File = "New Asset";
        
        ImGui::Separator();
        
        FString FileName = FString(FileIcon) + " " + File;
        
        if (ImGui::BeginMenu(FileName.c_str()))
        {
            const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
            for (CFactory* Factory : Factories)
            {
                if (Factory->CanImport() || Factory->GetAssetClass() == nullptr)
                {
                    continue;
                }
                
                FString DisplayName = Factory->GetAssetName();
                if (ImGui::MenuItem(DisplayName.c_str()))
                {
                    FFixedString Path = Paths::Combine(SelectedPath, Factory->GetDefaultAssetCreationName());
                    CPackage::AddPackageExt(Path);
                    Path = VFS::MakeUniqueFilePath(Path);
                    
                    if (Factory->HasCreationDialogue())
                    {
                        ToolContext->PushModal("Create New", {500, 500}, [this, Factory, Path = Move(Path)]
                        {
                            bool bShouldClose = CFactory::ShowCreationDialogue(Factory, Path);
                            if (bShouldClose)
                            {
                                ImGuiX::Notifications::NotifySuccess("Successfully Created: \"{0}\"", Path);
                            }
                    
                            return bShouldClose;
                        });
                    }
                    else
                    {
                        if (CObject* Object = Factory->TryCreateNew(Path))
                        {
                            CPackage::SavePackage(Object->GetPackage(), Path);
                            FAssetRegistry::Get().AssetCreated(Object);

                            ImGuiX::Notifications::NotifySuccess("Successfully Created: \"{0}\"", Path);
                        }
                        else
                        {
                            ImGuiX::Notifications::NotifyError("Failed to create new: \"{0}\"", Path);
        
                        }
                    }
                }
            }
            
            ImGui::EndMenu();
        }
    }
}
