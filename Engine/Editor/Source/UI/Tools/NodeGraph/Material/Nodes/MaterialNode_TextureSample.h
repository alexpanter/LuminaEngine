#pragma once
#include "MaterialNodeExpression.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "MaterialNode_TextureSample.generated.h"

namespace Lumina
{
    REFLECT()
    class CMaterialExpression_TextureSample : public CMaterialExpression
    {
        GENERATED_BODY()
    public:
        
        void BuildNode() override;
        FFixedString GetNodeCategory() const override { return "Textures"; }
        void* GetNodeDefaultValue() override { return &Texture; }
        FString GetNodeDisplayName() const override { return "TextureSample"; }
        uint32 GenerateExpression(FMaterialCompiler& Compiler) override;
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
        void SetNodeValue(void* Value) override;
        void DrawNodeBody() override;

        PROPERTY(Editable, Category = "Texture")
        TObjectPtr<CTexture> Texture;

        CMaterialInput* UV = nullptr;
    };
}
