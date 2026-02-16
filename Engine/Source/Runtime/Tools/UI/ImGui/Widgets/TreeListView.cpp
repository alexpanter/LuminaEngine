#include "pch.h"
#include "TreeListView.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiX.h"

namespace Lumina
{
    void FTreeListView::Draw(const FTreeListViewContext& Context)
    {
        if (bDirty)
        {
            RebuildTree(Context);
            return;
        }
        
        ImGuiTableFlags TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY;
        TableFlags |= ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersV;
        
        ImGui::PushID(this);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
        if (ImGui::BeginTable("TreeViewTable", 1, TableFlags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);

            auto View = Registry.view<FTreeNode, FRootNode>();
            ImGuiListClipper Clipper;
            
            Clipper.Begin(static_cast<int>(View.size_hint()));
            
            while (Clipper.Step())
            {
                for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
                {
                    const entt::basic_sparse_set<>* Set = View.handle();

                    entt::entity ItemEntity = (*Set)[i];
                    
                    if (!Set->contains(ItemEntity))
                    {
                        continue;
                    }
                    
                    if (Context.FilterFunction)
                    {
                        if (!Context.FilterFunction(*this, ItemEntity))
                        {
                            continue;
                        }
                    }
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    DrawListItem(ItemEntity, Context);
                }
            }

            ImGui::EndTable();
        }
        
        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    void FTreeListView::ClearTree()
    {
        Registry.clear<>();
    }

    void FTreeListView::RebuildTree(const FTreeListViewContext& Context)
    {
        DEBUG_ASSERT(Context.RebuildTreeFunction);
        DEBUG_ASSERT(bDirty);
        
        THashSet<uint64> CachedExpandedItems;
        auto BeforeView = Registry.view<FTreeNodeState, FTreeNode>();
        BeforeView.each([&](const FTreeNodeState& State, const FTreeNode& Node)
        {
            if (State.bExpanded)
            {
                CachedExpandedItems.emplace(Node.Hash);
            }
        });

        ClearSelection();
        ClearTree();
        
        Context.RebuildTreeFunction(*this);
        
        auto AfterView = Registry.view<FTreeNodeState, FTreeNode>();
        AfterView.each([&](FTreeNodeState& State, const FTreeNode& Node)
        {
            if (CachedExpandedItems.find(Node.Hash) != CachedExpandedItems.end())
            {
                State.bExpanded = true;
            }
        });

        bDirty = false;
    }

    void FTreeListView::DrawListItem(entt::entity Entity, const FTreeListViewContext& Context)
    {
        const FTreeNode& TreeNode       = Registry.get<FTreeNode>(Entity);
        const FTreeNodeDisplay& Display = Registry.get<FTreeNodeDisplay>(Entity);
        FTreeNodeState& State           = Registry.get<FTreeNodeState>(Entity);
        bool bHasChildren               = !TreeNode.Children.empty();
        
        ImGui::PushID(static_cast<int>(entt::to_integral(Entity)));

        ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_AllowOverlap;

        if (!bHasChildren)
        {
            Flags |= ImGuiTreeNodeFlags_Leaf;
        }
        else
        {
            ImGui::SetNextItemOpen(State.bExpanded);
            Flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        }

        if (State.bSelected)
        {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        
        FFixedString DisplayName = Display.DisplayName.c_str();
        if (bHasChildren)
        {
            DisplayName.append(" (");
            DisplayName.append(eastl::to_string(TreeNode.Children.size()).c_str());
            DisplayName.append(")");
        }
        
        ImVec4 TextColor = Display.DisplayColor;
        if (State.bDisabled)
        {
            TextColor.w *= 0.4f;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
        
        State.bExpanded = ImGui::TreeNodeEx("##TreeNode", Flags, "%s", DisplayName.c_str());
        
        if (ImGui::IsItemHovered() && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            SetSelection(Entity, Context);
        }
        
        if (ImGui::IsItemHovered())
        {
            for (int Key = ImGuiKey_NamedKey_BEGIN; Key < ImGuiKey_NamedKey_END; Key++)
            {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(Key)))
                {
                    if (HandleKeyPressed(Context, Entity, (ImGuiKey)Key))
                    {
                        break;
                    }
                }
            }
        }
        
        if (ImGui::BeginDragDropSource())
        {
            if (Context.SetDragDropFunction)
            {
                Context.SetDragDropFunction(*this, Entity);
            }
            
            ImGui::TextUnformatted(DisplayName.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (Context.DragDropFunction)
            {
                Context.DragDropFunction(*this, Entity);
            }

            ImGui::EndDragDropTarget();
        }
        
        ImGuiX::ItemTooltip("{}", Display.TooltipText);
        

        if (Context.ItemContextMenuFunction)
        {
            if (ImGui::BeginPopupContextItem("ItemContextMenu"))
            {
                if (Context.ItemContextMenuFunction)
                {
                    Context.ItemContextMenuFunction(*this, Entity);
                }
                
                ImGui::EndPopup();
            }
        }
        
        if (Display.bShowDisabledIcon)
        {
            ImGui::SameLine();
            float AvailableWidth = ImGui::GetContentRegionAvail().x;
            float ButtonWidth = 40.0f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + AvailableWidth - ButtonWidth + ImGui::GetStyle().FramePadding.x);
        
            const char* Icon = State.bDisabled ? LE_ICON_EYE_OFF : LE_ICON_EYE;
            if (ImGui::SmallButton(Icon))
            {
                State.bDisabled = !State.bDisabled;
                if (Context.VisibilityToggleFunction)
                {
                    Context.VisibilityToggleFunction(*this, Entity);
                }
            }
        }
        
        if (State.bExpanded)
        {
            for (entt::entity Child : TreeNode.Children)
            {
                ImGui::Indent(8);

                DrawListItem(Child, Context);
                
                ImGui::Unindent(8);
            }
            
            ImGui::TreePop();
        }
        
        ImGui::PopStyleColor();
        
        ImGui::PopID();
        
    }

    entt::entity FTreeListView::CreateNode(entt::entity Parent, const FFixedString& Name, uint64 Hash)
    {
        entt::entity NewNode = Registry.create();
        FTreeNode& TreeNode = Registry.emplace<FTreeNode>(NewNode);
        TreeNode.Hash = (Hash == 0) ? Hash::GetHash64(Name) : Hash;
        TreeNode.Parent = Parent;
        
        if (Parent != entt::null && Registry.valid(Parent))
        {
            FTreeNode& ParentNode = Registry.get<FTreeNode>(Parent);
            ParentNode.Children.push_back(NewNode);
        }
        
        FTreeNodeDisplay& Display = Registry.emplace<FTreeNodeDisplay>(NewNode);
        Display.DisplayName = Name;
        Display.TooltipText = Name;
        Registry.emplace<FTreeNodeState>(NewNode);
        
        if (Parent == entt::null)
        {
            Registry.emplace<FRootNode>(NewNode);
        }
        else
        {
            Registry.emplace<FLeafNode>(NewNode);            
        }
        
        return NewNode;
    }

    void FTreeListView::SetSelection(entt::entity Item, const FTreeListViewContext& Context)
    {
        auto View = Registry.view<FTreeNodeState>();
        View.each([&](entt::entity ViewEntity, FTreeNodeState& State)
        {
            if (ViewEntity == Item)
            {
                State.bSelected = true;
                if (Context.ItemSelectedFunction)
                {
                    Context.ItemSelectedFunction(*this, Item);
                }
                return;
            }
            State.bSelected = false;
        });
    }

    bool FTreeListView::HandleKeyPressed(const FTreeListViewContext& Context, entt::entity Item, ImGuiKey Key)
    {
        if (Context.KeyPressedFunction)
        {
            return Context.KeyPressedFunction(*this, Item, Key);
        }

        return false;
    }
    
    void FTreeListView::ClearSelection()
    {

    }
}
