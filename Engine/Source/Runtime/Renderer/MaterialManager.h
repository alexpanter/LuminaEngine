#pragma once

#include "RenderResource.h"
#include "MaterialTypes.h"
#include "Containers/Array.h"

namespace Lumina
{
    class CMaterialInterface;
    struct FMaterialUniforms;
}

namespace Lumina::RHI
{
    class FMaterialManager
    {
    public:
        
        FMaterialManager();
        
        void AddMaterial(CMaterialInterface* Material);
        void RemoveMaterial(CMaterialInterface* Material);
        
        void UpdateMaterialUniforms(CMaterialInterface* Material);
        FRHIBuffer* GetMaterialBuffer() const { return MaterialBuffer; }
    
    private:
        
        FSharedMutex                            Mutex;
        THashMap<int32, CMaterialInterface*>    MaterialMap;
        TVector<FMaterialUniforms>              MaterialUniforms;
        FRHIBufferRef                           MaterialBuffer;
    };
}
