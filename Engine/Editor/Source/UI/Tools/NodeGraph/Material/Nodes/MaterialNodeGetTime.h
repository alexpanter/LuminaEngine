#pragma once
#include "MaterialNodeExpression.h"
#include "Core/Object/ObjectMacros.h"
#include "MaterialNodeGetTime.generated.h"



namespace Lumina
{
    REFLECT()
    class CMaterialNodeGetTime : public CMaterialExpression
    {
        GENERATED_BODY()
    public:

        FFixedString GetNodeCategory() const override { return "Utility"; }
        FString GetNodeDisplayName() const override { return "Time"; }
        void* GetNodeDefaultValue() override { return nullptr; }
        void GenerateDefinition(FMaterialCompiler& Compiler) override;
    
    };
}
