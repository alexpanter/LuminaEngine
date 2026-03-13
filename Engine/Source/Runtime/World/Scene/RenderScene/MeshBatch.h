#pragma once

#include <glm/glm.hpp>
#include "Containers/Array.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class FRHIBuffer;
    class CMaterialInterface;
    class FDeferredRenderScene;

    struct FMeshBatch
    {
        struct FElement
        {
            uint32          FirstIndex;
            uint32          NumIndices;
        };
        
        glm::mat4                   RenderTransform;
        
        TFixedVector<FElement, 1>   Elements;
        CMaterialInterface*         Material;
        
        size_t                      VertexBuffer;
        size_t                      IndexBuffer;
    };
}
