#pragma once
#include "MaterialGraphNode.h"
#include "UI/Tools/NodeGraph/Material/MaterialInput.h"
#include "UI/Tools/NodeGraph/Material/MaterialOutput.h"
#include "UI/Tools/NodeGraph/Material/MaterialCompiler.h"
#include "MaterialNodeExpression.generated.h"

namespace Lumina
{
    REFLECT()
    class CMaterialExpression : public CMaterialGraphNode
    {
        GENERATED_BODY()
        
    public:
        
        void BuildNode() override;
        
        CMaterialOutput* Output;

        PROPERTY(Editable, Category = "Dynamic")
        bool bDynamic = false;
        
    };

    REFLECT()
    class CMaterialExpression_Math : public CMaterialExpression
    {
        GENERATED_BODY()
        
    public:
        void BuildNode() override;
        
        FFixedString GetNodeCategory() const override { return "Math"; }
        
        CMaterialInput* A = nullptr;
        CMaterialInput* B = nullptr;

        PROPERTY(Editable, Category = "Value")
        float ConstA = 0;

        PROPERTY(Editable, Category = "Value")
        float ConstB = 0;
    };

    REFLECT()
    class CMaterialExpression_Addition : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Add"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };

    REFLECT()
    class CMaterialExpression_Clamp : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Clamp"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        CMaterialInput* X = nullptr;
        
    };

    REFLECT()
    class CMaterialExpression_Saturate : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Saturate"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };
    
    REFLECT()
    class CMaterialExpression_Normalize : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Normalize"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };

    REFLECT()
    class CMaterialExpression_Distance : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Distance"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };

    REFLECT()
    class CMaterialExpression_Abs : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Abs"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };
    
    REFLECT()
    class CMaterialExpression_SmoothStep : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "SmoothStep"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        CMaterialInput* C = nullptr;

        
        PROPERTY(Editable, Category = "Value")
        float X = 0.5f;
        
    };
    
    REFLECT()
    class CMaterialExpression_Subtraction : public CMaterialExpression_Math
    {
        GENERATED_BODY()
        
    public:
        
        void BuildNode() override;


        FString GetNodeDisplayName() const override { return "Subtract"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };

    REFLECT()
    class CMaterialExpression_Multiplication : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Multiply"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };
    

    REFLECT()
    class CMaterialExpression_Sin : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Sin"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Cosin : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Cosin"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Floor : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Floor"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };
    
    REFLECT()
    class CMaterialExpression_Fract : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Fract"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Ceil : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Ceil"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Power : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Power"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Mod : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Mod"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Min : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Min"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Max : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Max"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

    };

    REFLECT()
    class CMaterialExpression_Step : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Step"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };
    
    REFLECT()
    class CMaterialExpression_Lerp : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Lerp"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        PROPERTY(Editable, Category = "Value")
        float Alpha = 0;

        
        CMaterialInput* C = nullptr;
    };
    
    
    REFLECT()
    class CMaterialExpression_Division : public CMaterialExpression_Math
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FString GetNodeDisplayName() const override { return "Divide"; }
        void* GetNodeDefaultValue() override { return &ConstA; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
    };


    //============================================================================================

    REFLECT()
    class CMaterialExpression_ComponentMask : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override;
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        ImVec2 GetMinNodeTitleBarSize() const override;

        FString BuildSwizzleMask() const;
        int32 GetSelectedComponentCount() const;
        EComponentMask GetOutputMask() const;
        FString GetDefaultValueForMask() const;

        CMaterialInput* InputPin = nullptr;
        
        PROPERTY(Editable)
        bool R = true;

        PROPERTY(Editable)
        bool G = true;

        PROPERTY(Editable)
        bool B = true;

        PROPERTY(Editable)
        bool A = true;
        
    };

    REFLECT()
    class CMaterialExpression_Append : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "Append"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        
        CMaterialInput* InputA = nullptr;
        CMaterialInput* InputB = nullptr;        
    };
    
    REFLECT()
    class CMaterialExpression_MakeFloat2 : public CMaterialExpression
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "MakeFloat2"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialInput* R = nullptr;
        CMaterialInput* G = nullptr;
    };
    
    REFLECT()
    class CMaterialExpression_MakeFloat3 : public CMaterialExpression
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "MakeFloat3"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialInput* R = nullptr;
        CMaterialInput* G = nullptr;
        CMaterialInput* B = nullptr;
    };
    
    REFLECT()
    class CMaterialExpression_MakeFloat4 : public CMaterialExpression
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "MakeFloat4"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialInput* R = nullptr;
        CMaterialInput* G = nullptr;
        CMaterialInput* B = nullptr;
        CMaterialInput* A = nullptr;
    };

    REFLECT()
    class CMaterialExpression_BreakFloat2 : public CMaterialExpression
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "BreakFloat2"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialInput* InputPin = nullptr;
        CMaterialOutput* R = nullptr;
        CMaterialOutput* G = nullptr;
    };

    REFLECT()
    class CMaterialExpression_BreakFloat3 : public CMaterialExpression_BreakFloat2
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "BreakFloat3"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialOutput* B = nullptr;
    };

    REFLECT()
    class CMaterialExpression_BreakFloat4 : public CMaterialExpression_BreakFloat3
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "BreakFloat4"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;


        CMaterialOutput* A = nullptr;
    };

    REFLECT()
    class CMaterialExpression_WorldPos : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "WorldPosition"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };

    REFLECT()
    class CMaterialExpression_CameraPos : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "CameraPosition"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };

    REFLECT()
    class CMaterialExpression_EntityID : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "EntityID"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };

    REFLECT()
    class CMaterialExpression_VertexNormal : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "VertexNormal"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    };
    
    REFLECT()
    class CMaterialExpression_Panner : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "Panner"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
        
        CMaterialInput* UV = nullptr;
        CMaterialInput* Time = nullptr;
        CMaterialInput* Speed = nullptr;

        PROPERTY(Editable)
        float SpeedX = 1.0f;
        
        PROPERTY(Editable)
        float SpeedY = 1.0f;
    };

    REFLECT()
    class CMaterialExpression_TexCoords : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "TexCoords"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        
        PROPERTY(Editable)
        uint32 TextureIndex = 0;
        
        PROPERTY(Editable)
        float UTiling = 1.0f;
        
        PROPERTY(Editable)
        float VTiling = 1.0f;
    };

    //============================================================================================


    REFLECT()
    class CMaterialExpression_Constant : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        
        FFixedString GetNodeCategory() const override { return "Constants"; }
        void DrawContextMenu() override;
        void DrawNodeTitleBar() override;
        void BuildNode() override;
        
        void* GetNodeDefaultValue() override { return &Value.r; }

        PROPERTY(Editable, Category = "Parameter")
        FName               ParameterName;

        PROPERTY(Editable, Color, Category = "Value")
        glm::vec4           Value = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        
        EMaterialInputType  ValueType = EMaterialInputType::Float;
    };

    REFLECT()
    class CMaterialExpression_ConstantFloat : public CMaterialExpression_Constant
    {
        GENERATED_BODY()
    public:
        
        CMaterialExpression_ConstantFloat()
        {
            ValueType = EMaterialInputType::Float;
        }

        FString GetNodeDisplayName() const override { return "Float"; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        void DrawNodeBody() override;
        
    };

    REFLECT()
    class CMaterialExpression_ConstantFloat2 : public CMaterialExpression_Constant
    {
        GENERATED_BODY()
    public:

        CMaterialExpression_ConstantFloat2()
        {
            ValueType = EMaterialInputType::Float2;
        }
        
        FString GetNodeDisplayName() const override { return "Float2"; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        void DrawNodeBody() override;
    };

    REFLECT()
    class CMaterialExpression_ConstantFloat3 : public CMaterialExpression_Constant
    {
        GENERATED_BODY()
    public:
        
        
        CMaterialExpression_ConstantFloat3()
        {
            ValueType = EMaterialInputType::Float3;
        }

        FString GetNodeDisplayName() const override { return "Float3"; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        void DrawNodeBody() override;
        
    };

    REFLECT()
    class CMaterialExpression_ConstantFloat4 : public CMaterialExpression_Constant
    {
        GENERATED_BODY()
    public:
        
        CMaterialExpression_ConstantFloat4()
        {
            ValueType = EMaterialInputType::Float4;
        }

        FString GetNodeDisplayName() const override { return "Float4"; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;

        void DrawNodeBody() override;

    };
}
