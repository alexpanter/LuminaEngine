#include "pch.h"
#include "Material.h"

#include "Assets/AssetTypes/Textures/Texture.h"
#include "Core/Engine/Engine.h"
#include "Core/Templates/AsBytes.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderManager.h"
#include "Renderer/RHIGlobals.h"

#include "Renderer/ShaderCompiler.h"
#include "Types/Byte.h"

namespace Lumina
{
    CMaterial* CMaterial::DefaultMaterial = nullptr;
    
    CMaterial::CMaterial()
    {
        MaterialType = EMaterialType::PBR;
        Memory::Memzero(&MaterialUniforms, sizeof(FMaterialUniforms));
    }

    void CMaterial::Serialize(FArchive& Ar)
    {
        CMaterialInterface::Serialize(Ar);
        Ar << PixelShaderBinaries;
    }

    void CMaterial::PostCreateCDO()
    {
        if (DefaultMaterial == nullptr)
        {
            CreateDefaultMaterial();
        }
    }

    void CMaterial::PostLoad()
    {
        if (!PixelShaderBinaries.empty())
        {
            FShaderHeader Header;
            StaticVertexShader = FShaderLibrary::GetVertexShader("GeometryPass.vert");
            
            TVector<FString> Defs{"SKINNED_VERTEX"};
            SkinnedVertexShader = FShaderLibrary::GetVertexShader("GeometryPass.vert", Defs);
            
            Header.DebugName = GetName().ToString() + "_PixelShader";
            Header.Hash = Hash::GetHash64(PixelShaderBinaries.data(), PixelShaderBinaries.size() * sizeof(uint32));
            Header.Binaries = PixelShaderBinaries;
            PixelShader = GRenderContext->CreatePixelShader(Header);

            FBindingSetDesc SetDesc;

            uint32 Index = 0;
            for (CTexture* Binding : Textures)
            {
                if (Binding != nullptr)
                {
                    MaterialUniforms.Textures[Index] = Binding->GetRHIRef()->GetTextureCacheIndex();
                    Index++;   
                }
            }
            
            if (GetMaterialIndex() == -1)
            {
                GRenderManager->GetMaterialManager().AddMaterial(this);
            }
            else
            {
                GRenderManager->GetMaterialManager().UpdateMaterialUniforms(this);
            }
            
            SetReadyForRender(true);
        }
    }

    void CMaterial::OnDestroy()
    {
        CMaterialInterface::OnDestroy();
        
        if (GetMaterialIndex() != -1)
        {
            GRenderManager->GetMaterialManager().RemoveMaterial(this);
        }
    }

    bool CMaterial::SetScalarValue(const FName& Name, const float Value)
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

    bool CMaterial::SetVectorValue(const FName& Name, const glm::vec4& Value)
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

    bool CMaterial::GetParameterValue(EMaterialParameterType Type, const FName& Name, FMaterialParameter& Param)
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

    CMaterial* CMaterial::GetMaterial() const
    {
        return const_cast<CMaterial*>(this);
    }
    
    FRHIVertexShader* CMaterial::GetVertexShader(EVertexFormat Format) const
    {
        switch (Format)
        {
            case EVertexFormat::Static: return StaticVertexShader;
            case EVertexFormat::Skinned: return SkinnedVertexShader;
        }
        UNREACHABLE();
    }

    FRHIPixelShader* CMaterial::GetPixelShader() const
    {
        return PixelShader;
    }

    CMaterial* CMaterial::GetDefaultMaterial()
    {
        return DefaultMaterial;
    }

    void CMaterial::CreateDefaultMaterial()
    {
        IShaderCompiler* ShaderCompiler = GRenderContext->GetShaderCompiler();

        ShaderCompiler->Flush();

        FString FragmentPath = Paths::GetEngineResourceDirectory() + "/Shaders/MaterialShader/ForwardBasePass.frag";
        
        if (DefaultMaterial)
        {
            DefaultMaterial->RemoveFromRoot();
            DefaultMaterial->ConditionalBeginDestroy();
            DefaultMaterial = nullptr;
        }
        
        DefaultMaterial = NewObject<CMaterial>(nullptr, "DefaultMaterial");
        DefaultMaterial->AddToRoot();
        
        FString LoadedString;
        if (!FileHelper::LoadFileIntoString(LoadedString, FragmentPath))
        {
            LOG_ERROR("Failed to find ForwardBasePass.frag!");
            return;
        }

        const char* Token = "$MATERIAL_INPUTS";
        size_t Pos = LoadedString.find(Token);

        FString Replacement;
        
        Replacement += "FMaterialInputs GetMaterialInputs()\n{\n";
        Replacement += "\tFMaterialInputs Input;\n";

        Replacement += "Input.Diffuse = vec3(1.0);\n";
        Replacement += "Input.Metallic = 0.0;\n";
        Replacement += "Input.Roughness = 1.0;\n";
        Replacement += "Input.Specular = 0.5;\n";
        Replacement += "Input.Emissive = vec3(0.0);\n";
        Replacement += "Input.AmbientOcclusion = 1.0;\n";
        Replacement += "Input.Normal = vec3(0.0, 0.0, 1.0);\n";
        Replacement += "Input.Opacity = 1.0;\n";
        Replacement += "Input.WorldPositionOffset = vec3(0.0);\n";

        Replacement += "\treturn Input;\n}\n";
        
        if (Pos != FString::npos)
        {
            LoadedString.replace(Pos, strlen(Token), Replacement);
        }
        else
        {
            LOG_ERROR("Missing [$MATERIAL_INPUTS] in base shader!");
        }
        
        ShaderCompiler->CompilerShaderRaw(LoadedString, {}, [](const FShaderHeader& Header) mutable 
        {
            DefaultMaterial->PixelShader = GRenderContext->CreatePixelShader(Header);
            DefaultMaterial->StaticVertexShader = GRenderContext->GetShaderLibrary()->GetShader("GeometryPass.vert").As<FRHIVertexShader>();

            DefaultMaterial->PixelShaderBinaries.assign(Header.Binaries.begin(), Header.Binaries.end());
            GRenderContext->OnShaderCompiled(DefaultMaterial->PixelShader, false, true);
        });

        ShaderCompiler->Flush();

        DefaultMaterial->PostLoad();
    }
}
