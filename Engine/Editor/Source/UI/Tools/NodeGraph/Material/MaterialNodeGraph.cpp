#include "MaterialNodeGraph.h"

#include "MaterialCompiler.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "Core/Object/Cast.h"
#include "Nodes/MaterialNodeExpression.h"
#include "Nodes/MaterialNodeGetTime.h"
#include "Nodes/MaterialNode_PrimitiveData.h"
#include "Nodes/MaterialNode_TextureSample.h"
#include "Nodes/MaterialOutputNode.h"

#include "UI/Tools/NodeGraph/EdNodeGraphPin.h"
#include "UI/Tools/NodeGraph/EdNode_Reroute.h"


namespace Lumina
{
    CMaterialNodeGraph::CMaterialNodeGraph()
    {
        
    }

    void CMaterialNodeGraph::Initialize()
    {
        Super::Initialize();
        
        bool bHasOutputNode = false;
        auto NewNodes = Move(Nodes);
        auto NewConnections = Move(Connections);
        Nodes.clear();
        Connections.clear();
        
        for (CObject* Object : NewNodes)
        {
            CEdGraphNode* Node = Cast<CEdGraphNode>(Object);
            if (Node == nullptr)
            {
                continue;
            }
            
            if (Node->IsA<CMaterialOutputNode>())
            {
                bHasOutputNode = true;
            }
            
            AddNode(Node);
        }

        if (!bHasOutputNode)
        {
            CreateNode(CMaterialOutputNode::StaticClass());
        }
        
        for (size_t i = 0; i < NewConnections.size(); i += 2)
        {
            uint16 FirstConnection = NewConnections[i];
            uint16 SecondConnection = NewConnections[i + 1];
            
            CEdNodeGraphPin* StartPin = nullptr;
            CEdNodeGraphPin* EndPin = nullptr;

            for (CEdGraphNode* Node : Nodes)
            {
                EndPin = Node->GetPin(FirstConnection, ENodePinDirection::Input);
                if (EndPin)
                {
                    break;
                }
            }
            
            for (CEdGraphNode* Node : Nodes)
            {
                StartPin = Node->GetPin(SecondConnection, ENodePinDirection::Output);
                if (StartPin)
                {
                    break;
                }
            }

            if (!StartPin || !EndPin || StartPin == EndPin || StartPin->OwningNode == EndPin->OwningNode)
            {
                continue;
            }

            if (EndPin->HasConnection())
            {
                continue; // Disallow connection if the input pin is already occupied
            }

            // Allow the connection
            StartPin->AddConnection(EndPin);
            EndPin->AddConnection(StartPin);
        }
        
        //RegisterGraphNode(CEdNode_Reroute::StaticClass());

        RegisterGraphNode(CMaterialExpression_SmoothStep::StaticClass());
        RegisterGraphNode(CMaterialExpression_Saturate::StaticClass());
        RegisterGraphNode(CMaterialExpression_Normalize::StaticClass());
        RegisterGraphNode(CMaterialExpression_Distance::StaticClass());
        RegisterGraphNode(CMaterialExpression_Abs::StaticClass());

        RegisterGraphNode(CMaterialExpression_Addition::StaticClass());
        RegisterGraphNode(CMaterialExpression_Subtraction::StaticClass());
        RegisterGraphNode(CMaterialExpression_Division::StaticClass());
        RegisterGraphNode(CMaterialExpression_Multiplication::StaticClass());
        RegisterGraphNode(CMaterialExpression_Sin::StaticClass());
        RegisterGraphNode(CMaterialExpression_Cosin::StaticClass());
        RegisterGraphNode(CMaterialExpression_Floor::StaticClass());
        RegisterGraphNode(CMaterialExpression_Fract::StaticClass());
        RegisterGraphNode(CMaterialExpression_Ceil::StaticClass());
        RegisterGraphNode(CMaterialExpression_Power::StaticClass());
        RegisterGraphNode(CMaterialExpression_Mod::StaticClass());
        RegisterGraphNode(CMaterialExpression_Min::StaticClass());
        RegisterGraphNode(CMaterialExpression_Max::StaticClass());
        RegisterGraphNode(CMaterialExpression_Step::StaticClass());
        RegisterGraphNode(CMaterialExpression_Lerp::StaticClass());
        RegisterGraphNode(CMaterialExpression_Clamp::StaticClass());

        //RegisterGraphNode(CMaterialExpression_Append::StaticClass());
        RegisterGraphNode(CMaterialExpression_ComponentMask::StaticClass());
        RegisterGraphNode(CMaterialExpression_VertexNormal::StaticClass());
        RegisterGraphNode(CMaterialExpression_TexCoords::StaticClass());
        RegisterGraphNode(CMaterialExpression_Panner::StaticClass());
        RegisterGraphNode(CMaterialNodeGetTime::StaticClass());
        RegisterGraphNode(CMaterialExpression_CameraPos::StaticClass());
        RegisterGraphNode(CMaterialExpression_WorldPos::StaticClass());
        RegisterGraphNode(CMaterialExpression_EntityID::StaticClass());
        RegisterGraphNode(CMaterialExpression_CustomPrimitiveData::StaticClass());

        RegisterGraphNode(CMaterialExpression_ConstantFloat::StaticClass());
        RegisterGraphNode(CMaterialExpression_ConstantFloat2::StaticClass());
        RegisterGraphNode(CMaterialExpression_ConstantFloat3::StaticClass());
        RegisterGraphNode(CMaterialExpression_ConstantFloat4::StaticClass());

        RegisterGraphNode(CMaterialExpression_BreakFloat2::StaticClass());
        RegisterGraphNode(CMaterialExpression_BreakFloat3::StaticClass());
        RegisterGraphNode(CMaterialExpression_BreakFloat4::StaticClass());

        RegisterGraphNode(CMaterialExpression_MakeFloat2::StaticClass());
        RegisterGraphNode(CMaterialExpression_MakeFloat3::StaticClass());
        RegisterGraphNode(CMaterialExpression_MakeFloat4::StaticClass());
        
        RegisterGraphNode(CMaterialExpression_TextureSample::StaticClass());

        ValidateGraph();
        
    }
    
    void CMaterialNodeGraph::Shutdown()
    {
        CEdNodeGraph::Shutdown();
    }
    
    void CMaterialNodeGraph::CompileGraph(FMaterialCompiler& Compiler)
    {
        TVector<CEdGraphNode*> SortedNodes;
        TVector<CEdGraphNode*> NodesToEvaluate;
        TSet<CEdGraphNode*> ReachableNodes;
        
        if (Nodes.empty())
        {
            return;
        }

        for (CEdGraphNode* Node : Nodes)
        {
            Node->ClearError();
        }
        
        CEdGraphNode* CyclicNode = TopologicalSort(Nodes, SortedNodes);

        if (CyclicNode != nullptr)
        {
            EdNodeGraph::FError Error;
            Error.Name          = "Cyclic";
            Error.Description   = "Cycle detected in material node graph! Graph must be acyclic!";
            Error.Node          = CyclicNode;
            Compiler.AddError(Error);
            
            return;
        }
        
        Compiler.NewLine();
        Compiler.NewLine();

        for (size_t i = 0; i < SortedNodes.size(); ++i)
        {
            CEdGraphNode* Node = SortedNodes[i];
            
            Node->SetDebugExecutionOrder((uint32)i);
            if (Node->GetClass() == CMaterialOutputNode::StaticClass())
            {
                continue; 
            }

            CMaterialGraphNode* MaterialGraphNode = static_cast<CMaterialGraphNode*>(Node);
            MaterialGraphNode->GenerateDefinition(Compiler);
        }

        // Start off the compilation process using the MaterialOutput node as the kick-off.
        CMaterialGraphNode* MaterialOutputNode = static_cast<CMaterialGraphNode*>(Nodes[0].Get());
        MaterialOutputNode->GenerateDefinition(Compiler);

        for (auto& Error : Compiler.GetErrors())
        {
            if (Error.Node)
            {
                Error.Node->SetError(Error);
            }
        }
    }

    void CMaterialNodeGraph::ValidateGraph()
    {
        Connections.clear();
        
        for (CEdGraphNode* Node : Nodes)
        {
            for (CEdNodeGraphPin* InputPin : Node->GetInputPins())
            {
                for (CEdNodeGraphPin* Connection : InputPin->GetConnections())
                {
                    Connections.push_back(InputPin->PinID);
                    Connections.push_back(Connection->PinID);
                }
            }
        }
    }

    void CMaterialNodeGraph::SetMaterial(CMaterial* InMaterial)
    {
        Material = InMaterial;
    }

    CEdGraphNode* CMaterialNodeGraph::CreateNode(CClass* NodeClass)
    {
        CEdGraphNode* NewNode = NewObject<CEdGraphNode>(NodeClass, Material->GetPackage());
        AddNode(NewNode);
        return NewNode;
    }
    
    CEdGraphNode* CMaterialNodeGraph::TopologicalSort(const TVector<TObjectPtr<CEdGraphNode>>& NodesToSort, TVector<CEdGraphNode*>& SortedNodes)
    {
        THashMap<CEdGraphNode*, uint32> InDegree;
        TQueue<CEdGraphNode*> ReadyQueue;
        THashSet<CEdGraphNode*> ReachableNodes;
        uint32 ProcessedNodeCount = 0;
    
        CEdGraphNode* RootNode = nullptr;
        for (CEdGraphNode* Node : NodesToSort)
        {
            if (Cast<CMaterialOutputNode>(Node))
            {
                RootNode = Node;
                break;
            }
        }
    
        if (!RootNode)
        {
            SortedNodes.clear();
            return nullptr;
        }
    
        TQueue<CEdGraphNode*> ReverseQueue;
        ReverseQueue.push(RootNode);
        ReachableNodes.insert(RootNode);
    
        while (!ReverseQueue.empty())
        {
            CEdGraphNode* Node = ReverseQueue.front();
            ReverseQueue.pop();
    
            for (CEdNodeGraphPin* InputPin : Node->GetInputPins())
            {
                for (CEdNodeGraphPin* ConnectedPin : InputPin->GetConnections())
                {
                    CEdGraphNode* ConnectedNode = ConnectedPin->GetOwningNode();
                    if (ReachableNodes.insert(ConnectedNode).second)
                    {
                        ReverseQueue.push(ConnectedNode);
                    }
                }
            }
        }
    
        for (CEdGraphNode* Node : NodesToSort)
        {
            if (ReachableNodes.find(Node) != ReachableNodes.end())
            {
                InDegree[Node] = 0;
            }
        }
    
        for (CEdGraphNode* Node : NodesToSort)
        {
            if (ReachableNodes.find(Node) == ReachableNodes.end())
            {
                continue;
            }

            for (CEdNodeGraphPin* OutputPin : Node->GetOutputPins())
            {
                for (CEdNodeGraphPin* ConnectedPin : OutputPin->GetConnections())
                {
                    CEdGraphNode* ConnectedNode = ConnectedPin->GetOwningNode();
                    if (ReachableNodes.find(ConnectedNode) != ReachableNodes.end())
                    {
                        InDegree[ConnectedNode]++;
                    }
                }
            }
        }
    
        for (auto& Pair : InDegree)
        {
            if (Pair.second == 0)
            {
                ReadyQueue.push(Pair.first);
            }
        }
    
        while (!ReadyQueue.empty())
        {
            CEdGraphNode* Node = ReadyQueue.front();
            ReadyQueue.pop();
            SortedNodes.push_back(Node);
            ProcessedNodeCount++;
    
            for (CEdNodeGraphPin* OutputPin : Node->GetOutputPins())
            {
                for (CEdNodeGraphPin* ConnectedPin : OutputPin->GetConnections())
                {
                    CEdGraphNode* ConnectedNode = ConnectedPin->GetOwningNode();
                    if (ReachableNodes.find(ConnectedNode) == ReachableNodes.end())
                    {
                        continue;
                    }

                    if (--InDegree[ConnectedNode] == 0)
                    {
                        ReadyQueue.push(ConnectedNode);
                    }
                }
            }
        }
    
        if (ProcessedNodeCount != ReachableNodes.size())
        {
            for (auto& Pair : InDegree)
            {
                if (Pair.second > 0)
                {
                    SortedNodes.clear();
                    return Pair.first;
                }
            }
        }
    
        return nullptr;
    }
}
