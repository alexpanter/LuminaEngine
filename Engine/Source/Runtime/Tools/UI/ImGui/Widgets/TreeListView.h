#pragma once
#include "imgui.h"
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Containers/Function.h"
#include "Core/Threading/Atomic.h"
#include "entt/entt.hpp"

namespace Lumina
{
    class FTreeListView;

    struct RUNTIME_API FTreeNode
    {
        entt::entity Parent;
        TVector<entt::entity> Children;
        uint64 Hash;
        uint32 Depth;
        uint32 VisibleIndex;
    };
    
    struct RUNTIME_API FTreeNodeDisplay
    {
        FName           DisplayName;
        FFixedString    TooltipText;
        ImVec4          DisplayColor = ImVec4(0.725f, 0.725f, 0.725f, 1.0f);
        
        bool            bShowDisabledIcon = false;

    };
    
    struct RUNTIME_API FTreeNodeState
    {
        uint8 bExpanded:1       = false;
        uint8 bVisible:1        = false;
        uint8 bSelected:1       = false;
        uint8 bPassesFilter:1   = false;
        uint8 bDisabled:1       = false;
    };
    
    struct FRootNode {};
    struct FLeafNode {};
    
    
    struct RUNTIME_API FTreeListViewContext
    {
        /** Check if the item to draw passes a filter */
        TFunction<bool(FTreeListView&, entt::entity)>                   FilterFunction;
        
        /** Callback to draw any context menus this item may want */
        TFunction<void(FTreeListView&, entt::entity)>                   ItemContextMenuFunction;

        /** Called when a rebuild of the widget tree is requested */
        TFunction<void(FTreeListView&)>                                 RebuildTreeFunction;

        /** Called when an item has been selected in the tree */
        TFunction<void(FTreeListView&, entt::entity)>                   ItemSelectedFunction;

        /** Called when we have a drag-drop operation on a target */
        TFunction<void(FTreeListView&, entt::entity)>                   DragDropFunction;
        
        /** Set drag drop payload */
        TFunction<void(FTreeListView&, entt::entity)>                   SetDragDropFunction;
        
        /** Called when a key is pressed while hovering the tile item, return true to absorb. */
        TFunction<bool(FTreeListView&, entt::entity, ImGuiKey)>         KeyPressedFunction;
        
        /** Called when the visibility icon is toggled */
        TFunction<void(FTreeListView&, entt::entity)>                   VisibilityToggleFunction;
    };
    
    
    class RUNTIME_API FTreeListView
    {
    public:

        FTreeListView() = default;
        ~FTreeListView() = default;
        LE_NO_COPYMOVE(FTreeListView);
        
        void Draw(const FTreeListViewContext& Context);

        void ClearTree();

        void MarkTreeDirty() { bDirty = true; }
        NODISCARD bool IsDirty() const { return bDirty; }
        
        FORCEINLINE entt::registry& GetRegistry() { return Registry; }
        entt::entity CreateNode(entt::entity Parent, const FFixedString& Name, uint64 Hash = 0);
        
        template<typename T, typename ... TArgs>
        T& EmplaceUserData(entt::entity Entity, TArgs&&... Args)
        {
            return Registry.emplace<T>(Entity, eastl::forward<TArgs>(Args)...);
        }
        
        template<typename ... Ts>
        decltype(auto) Get(entt::entity Entity)
        {
            return Registry.get<Ts...>(Entity);
        }
    
    private:
        
        void SetSelection(entt::entity Item, const FTreeListViewContext& Context);

        bool HandleKeyPressed(const FTreeListViewContext& Context, entt::entity Item, ImGuiKey Key);
        
        void RebuildTree(const FTreeListViewContext& Context);
        
        void DrawListItem(entt::entity Entity, const FTreeListViewContext& Context);

        void ClearSelection();
    
    private:
        
        entt::registry  Registry;
        bool            bDirty = false;
        TAtomic<bool>   bRebuilding = false;
    };
}