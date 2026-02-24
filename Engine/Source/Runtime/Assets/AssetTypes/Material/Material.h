#pragma once

#include "MaterialInterface.h"
#include "Containers/Array.h"
#include "Core/Object/Object.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Object/ObjectMacros.h"
#include "Renderer/MaterialTypes.h"
#include "Renderer/RenderResource.h"
#include "Material.generated.h"

namespace Lumina
{
    class CTexture;
}

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CMaterial : public CMaterialInterface
    {
        GENERATED_BODY()
        
    public:

        CMaterial();

        void Serialize(FArchive& Ar) override;
        bool IsAsset() const override { return true; }
        void PostCreateCDO() override;
        void PostLoad() override;
        void OnDestroy() override;
        
        bool SetScalarValue(const FName& Name, const float Value) override;
        bool SetVectorValue(const FName& Name, const glm::vec4& Value) override;
        bool GetParameterValue(EMaterialParameterType Type, const FName& Name, FMaterialParameter& Param) override;
        FMaterialUniforms* GetMaterialUniforms() override { return &MaterialUniforms; }
        
        CMaterial* GetMaterial() const override;
        FRHIVertexShader* GetVertexShader() const override;
        FRHIPixelShader* GetPixelShader() const override;
        static CMaterial* GetDefaultMaterial();

        static void CreateDefaultMaterial();

        EMaterialType GetMaterialType() const override { return MaterialType; }
        bool DoesCastShadows() const override { return bCastShadows; }
        bool IsTwoSided() const override { return bTwoSided; }
        bool IsTranslucent() override { return bTranslucent; }
        
        PROPERTY(Editable)
        EMaterialType MaterialType;

        PROPERTY(Editable)
        bool bCastShadows = true;

        PROPERTY(Editable)
        bool bTwoSided = false;

        PROPERTY(Editable)
        bool bTranslucent = false;

        
        PROPERTY()
        TVector<TObjectPtr<CTexture>>           Textures;
        
        PROPERTY()
        TVector<uint32>                         PixelShaderBinaries;

        PROPERTY()
        TVector<uint32>                         VertexShaderBinaries;
        
        PROPERTY()
        TVector<FMaterialParameter>             Parameters;
        
        FMaterialUniforms                       MaterialUniforms;
        
        FRHIVertexShaderRef                     VertexShader;
        FRHIPixelShaderRef                      PixelShader;

    };
    
}
