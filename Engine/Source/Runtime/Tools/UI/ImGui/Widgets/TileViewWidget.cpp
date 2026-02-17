#include "pch.h"
#include "TileViewWidget.h"
#include "Tools/UI/ImGui/ImGuiX.h"

namespace Lumina
{
    void FTileViewWidget::Draw(const FTileViewContext& Context)
    {
        if (bDirty)
        {
            RebuildTree(Context);
            return;
        }
    
        float PaneWidth = ImGui::GetContentRegionAvail().x;
        constexpr float ThumbnailSize = 120.0f;
        constexpr float TileSpacing = 5.0f;
        constexpr float TextHeight = 36.0f;
        float CellSize = ThumbnailSize + TileSpacing;
        int ItemsPerRow = std::max(1, int(PaneWidth / CellSize));
    
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(TileSpacing, TileSpacing));
    
        int ItemIndex = 0;
        for (FTileViewItem* Item : ListItems)
        {
            const char* DisplayName = Item->GetDisplayName().c_str();
    
            if (ItemIndex % ItemsPerRow != 0)
            {
                ImGui::SameLine();
            }
    
            ImGui::PushID(Item);
            ImGui::BeginGroup();
    
            DrawItem(Item, Context, ImVec2(ThumbnailSize, ThumbnailSize));
    
            ImFont* Font = ImGui::GetIO().Fonts->Fonts[3];
            ImGui::PushFont(Font);
    
            float WrapWidth = ThumbnailSize;
            ImVec2 TextSize = ImGui::CalcTextSize(DisplayName, nullptr, false, WrapWidth);
            
            float TextPosX = (ThumbnailSize - std::min(TextSize.x, WrapWidth)) * 0.5f;
            
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TextPosX);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
            
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + WrapWidth);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            ImGui::TextWrapped("%s", DisplayName);
            ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();
    
            ImGui::PopFont();
    
            ImGui::Dummy(ImVec2(0, std::max(0.0f, TextHeight - TextSize.y)));
    
            ImGui::EndGroup();
            ImGui::PopID();
    
            ++ItemIndex;
        }
    
        ImGui::PopStyleVar();
    }
    
    void FTileViewWidget::ClearTree()
    {
        Allocator.Reset();
        ListItems.clear();
    }

    bool FTileViewWidget::HandleKeyPressed(const FTileViewContext& Context, FTileViewItem& Item, ImGuiKey Key)
    {
        if (Context.KeyPressedFunction)
        {
            return Context.KeyPressedFunction(Item, Key);
        }

        return false;
    }

    void FTileViewWidget::RebuildTree(const FTileViewContext& Context, bool bKeepSelections)
    {
        ASSERT(bDirty);

        TVector<FTileViewItem*> CachedSelections = Selections;
        
        ClearSelections();
        ClearTree();

        if (bKeepSelections)
        {
            for (FTileViewItem* Select : CachedSelections)
            {
                ToggleSelection(Select, Context);
            }
        }
        
        Context.RebuildTreeFunction(this);

        bDirty = false;
    }

    void FTileViewWidget::DrawItem(FTileViewItem* ItemToDraw, const FTileViewContext& Context, ImVec2 DrawSize)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        
        
        if (Context.DrawItemOverrideFunction)
        {
            FTileViewItem::EClickState ClickState = Context.DrawItemOverrideFunction(ItemToDraw);
            if (ClickState == FTileViewItem::EClickState::SingleWithCtrl)
            {
                ToggleSelection(ItemToDraw, Context);
            }
            else if (ClickState == FTileViewItem::EClickState::Single)
            {
                ClearSelections();
                ToggleSelection(ItemToDraw, Context);
            }
            else if (ClickState == FTileViewItem::EClickState::Double)
            {
                if (Context.ItemDoubleClickedFunction)
                {
                    Context.ItemDoubleClickedFunction(ItemToDraw);
                }
            }
        }
        else
        {
            if (ImGui::Button("##", DrawSize))
            {
                ClearSelections();
                ToggleSelection(ItemToDraw, Context);
            }
        }
        
        ImGui::PopStyleVar(2);
    
        if (ImGui::BeginItemTooltip())
        {
            ItemToDraw->DrawTooltip();
            ImGui::EndTooltip();
        }
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ItemToDraw->HasContextMenu())
        {
            ImGui::OpenPopup("ItemContextMenu");
        }

        if (ImGui::IsItemHovered())
        {
            for (int Key = ImGuiKey_NamedKey_BEGIN; Key < ImGuiKey_NamedKey_END; Key++)
            {
                if (ImGui::IsKeyPressed((ImGuiKey)Key))
                {
                    if (HandleKeyPressed(Context, *ItemToDraw, (ImGuiKey)Key))
                    {
                        break;
                    }
                }
            }
        }
        
        if (ImGui::BeginDragDropSource())
        {
            ItemToDraw->SetDragDropPayloadData();
            if (Context.DrawItemOverrideFunction)
            {
                Context.DrawItemOverrideFunction(ItemToDraw);
            }
            ImGui::EndDragDropSource();
        }
    
        if (ImGui::BeginDragDropTarget())
        {
            if (Context.DragDropFunction)
            {
                Context.DragDropFunction(ItemToDraw, Selections);
            }
            
            ImGui::EndDragDropTarget();
        }
        
        if (ItemToDraw->HasContextMenu())
        {
            if (ImGui::BeginPopupContextItem("ItemContextMenu"))
            {
                TVector<FTileViewItem*> SelectionsToDraw;
                SelectionsToDraw.push_back(ItemToDraw);
                Context.DrawItemContextMenuFunction(SelectionsToDraw);
                
                ImGui::EndPopup();
            }
        }
    }

    void FTileViewWidget::ToggleSelection(FTileViewItem* Item, const FTileViewContext& Context)
    {
        bool bWasSelected = Item->bSelected;
        
        if (!bWasSelected)
        {
            DEBUG_ASSERT(eastl::find(Selections.begin(), Selections.end(), Item) == Selections.end());
            Selections.push_back(Item);
            Context.ItemSelectedFunction(Item);
            Item->bSelected = true;
        }
        else
        {
            auto It = eastl::remove(Selections.begin(), Selections.end(), Item);
            Selections.erase(It);
            Item->bSelected = false;
        }

        Item->OnSelectionStateChanged();
    }

    void FTileViewWidget::ClearSelections()
    {
        for (FTileViewItem* Item : Selections)
        {
            ASSERT(Item->bSelected);
            
            Item->bSelected = false;
            Item->OnSelectionStateChanged();    
        }

        Selections.clear();
    }
}
