#include "pch.h"
#include "MaterialInstance.h"
#include "Material.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "Core/Engine/Engine.h"
#include "Core/Templates/AsBytes.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderManager.h"
#include "Renderer/RHIGlobals.h"


namespace Lumina
{
    enum class Byte : uint8;

    CMaterialInstance::CMaterialInstance()
    {
        Memory::Memzero(&MaterialUniforms, sizeof(FMaterialUniforms));
        if (Material)
        {
            MaterialUniforms = Material->MaterialUniforms;
            Parameters = Material->Parameters;
        }
    }

    CMaterial* CMaterialInstance::GetMaterial() const
    {
        return Material.Get();
    }

    bool CMaterialInstance::SetScalarValue(const FName& Name, const float Value)
    {
        auto* Itr = eastl::find_if(Parameters.begin(), Parameters.end(), [Name](const FMaterialParameter& Param)
        {
           return Param.ParameterName == Name && Param.Type == EMaterialParameterType::Scalar; 
        });

        if (Itr != Parameters.end())
        {
            const FMaterialParameter& Param = *Itr;
            MaterialUniforms.Scalars[Param.Index] = Value;
            return true;
        }
        else
        {
            LOG_ERROR("Failed to find material scalar parameter {}", Name);
        }

        return false;
    }

    bool CMaterialInstance::SetVectorValue(const FName& Name, const glm::vec4& Value)
    {
        auto* Itr = eastl::find_if(Parameters.begin(), Parameters.end(), [Name](const FMaterialParameter& Param)
        {
           return Param.ParameterName == Name && Param.Type == EMaterialParameterType::Vector; 
        });

        if (Itr != Parameters.end())
        {
            const FMaterialParameter& Param = *Itr;
            MaterialUniforms.Vectors[Param.Index] = Value;
            return true;
        }
        else
        {
            LOG_ERROR("Failed to find material vector parameter {}", Name);
        }

        return false;
    }

    bool CMaterialInstance::GetParameterValue(EMaterialParameterType Type, const FName& Name, FMaterialParameter& Param)
    {
        Param = {};
        auto* Itr = eastl::find_if(Parameters.begin(), Parameters.end(), [Type, Name](const FMaterialParameter& Param)
        {
           return Param.ParameterName == Name && Param.Type == Type; 
        });

        if (Itr != Parameters.end())
        {
            Param = *Itr;
            return true;
        }
        
        return false;
    }
    

    FRHIVertexShader* CMaterialInstance::GetVertexShader(EVertexFormat Format) const
    {
        return Material->GetVertexShader(Format);
    }

    FRHIPixelShader* CMaterialInstance::GetPixelShader() const
    {
        return Material->GetPixelShader();
    }

    void CMaterialInstance::PostLoad()
    {
        if (Material)
        {
        }
    }
}
