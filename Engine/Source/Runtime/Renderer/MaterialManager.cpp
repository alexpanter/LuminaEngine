#include "pch.h"
#include "MaterialManager.h"
#include "MaterialTypes.h"
#include "RenderContext.h"
#include "RHIGlobals.h"
#include "Assets/AssetTypes/Material/MaterialInterface.h"
#include "Memory/Memcpy.h"

namespace Lumina::RHI
{
    FMaterialManager::FMaterialManager()
    {
        FRHIBufferDesc Desc;
        Desc.Size = sizeof(FMaterialUniforms) * 2000;
        Desc.Stride = sizeof(FMaterialUniforms);
        Desc.DebugName = "Material Uniforms";
        Desc.InitialState = EResourceStates::UnorderedAccess;
        Desc.bKeepInitialState = true;
        
        MaterialBuffer = GRenderContext->CreateBuffer(Desc);
    }

    void FMaterialManager::AddMaterial(CMaterialInterface* Material)
    {
        DEBUG_ASSERT(Material != nullptr);
        DEBUG_ASSERT(Material->GetMaterialIndex() == -1);
        
        Material->SetMaterialIndex(MaterialUniforms.size());
        MaterialMap.emplace(Material->GetMaterialIndex(), Material);
        MaterialUniforms.emplace_back(*Material->GetMaterialUniforms());
        UpdateMaterialUniforms(Material);
    }

    void FMaterialManager::RemoveMaterial(CMaterialInterface* Material)
    {
        DEBUG_ASSERT(Material != nullptr);
        DEBUG_ASSERT(Material->GetMaterialIndex() != -1);
    
        int32 MaterialIndex = Material->GetMaterialIndex();
        int32 LastMaterialIndex = MaterialUniforms.size() - 1;
    
        if (MaterialIndex != LastMaterialIndex)
        {
            CMaterialInterface* LastMaterial = MaterialMap[LastMaterialIndex];
        
            MaterialUniforms[MaterialIndex] = MaterialUniforms[LastMaterialIndex];
        
            LastMaterial->SetMaterialIndex(MaterialIndex);
        
            MaterialMap[MaterialIndex] = LastMaterial;
        }
    
        MaterialUniforms.pop_back();
    
        MaterialMap.erase(MaterialIndex);
        
        Material->SetMaterialIndex(-1);
    }

    void FMaterialManager::UpdateMaterialUniforms(CMaterialInterface* Material)
    {
        FMaterialUniforms& Uniforms = MaterialUniforms[Material->GetMaterialIndex()];
        Memory::Memcpy(&Uniforms, Material->GetMaterialUniforms(), sizeof(FMaterialUniforms));
        
        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        CommandList->Open();
        CommandList->WriteBuffer(MaterialBuffer, MaterialUniforms.data(), sizeof(FMaterialUniforms), Material->GetMaterialIndex() * sizeof(FMaterialUniforms));
        CommandList->Close();
        
        GRenderContext->ExecuteCommandList(CommandList);
    }
}
