#pragma once

#include "EdGraphNode.h"
#include "Containers/Array.h"
#include "Containers/Function.h"
#include "Containers/String.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "EdNodeGraph.generated.h"

namespace Lumina
{
    class CEdGraphNode;
}

namespace Lumina
{
    REFLECT()
    class CEdNodeGraph : public CObject
    {
        GENERATED_BODY()
        
    public:
        
        struct FAction
        {
            FString             ActionName;
            TFunction<void()>   ActionCallback;
        };

        struct FNodeFactory
        {
            FString Name;
            FString Tooltip;
            TFunction<CEdGraphNode*()> CreationCallback;
        };

        
        CEdNodeGraph();
        ~CEdNodeGraph() override;

        virtual void Initialize();
        virtual void Shutdown();
        void Serialize(FArchive& Ar) override;
        
        void DrawGraph();
        virtual void DrawGraphContextMenu();
        virtual void DrawNodeContextMenu(CEdGraphNode* Node);
        virtual void DrawPinContextMenu(CEdNodeGraphPin* Pin);
        
        virtual void ValidateGraph()  { }
        
        virtual CEdGraphNode* CreateNode(CClass* NodeClass);

        virtual CEdGraphNode* OnNodeRemoved(CEdGraphNode* Node) { return nullptr; }

        void SetNodeSelectedCallback(const TFunction<void(CEdGraphNode*)>& Callback) { NodeSelectedCallback = Callback; }
        void SetPreNodeDeletedCallback(const TFunction<void(CEdGraphNode*)>& Callback) { PreNodeDeletedCallback = Callback; }
        
        virtual bool CanCreateConnection(CEdNodeGraphPin* A, CEdNodeGraphPin* B) { return true; }


    private:

        static bool GraphSaveSettings(const char* data, size_t size, ax::NodeEditor::SaveReasonFlags reason, void* userPointer);
        static size_t GraphLoadSettings(char* data, void* userPointer);
        
    public:

        void RegisterGraphNode(CClass* InClass);
        
        uint64 AddNode(CEdGraphNode* InNode);

        PROPERTY()
        TVector<TObjectPtr<CEdGraphNode>>               Nodes;
        
        PROPERTY()
        TVector<uint16>                                 Connections;

        PROPERTY()
        FString GraphSaveData;
        
        THashSet<CClass*>                               SupportedNodes;
        
        TFunction<void(CEdGraphNode*)>                  NodeSelectedCallback;
        TFunction<void(CEdGraphNode*)>                  PreNodeDeletedCallback;

        int64                                           NextID = 0;

        
        ImGuiTextFilter                                 Filter;
    
        TVector<CEdGraphNode*>                          CopiedNodes;
        
        bool                                            bFirstDraw = true;
        bool                                            bDebug = false;
        
    private:

        ax::NodeEditor::EditorContext* Context = nullptr;
    };
    
    
}
