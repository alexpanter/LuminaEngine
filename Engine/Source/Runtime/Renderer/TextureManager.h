#pragma once

#include "DescriptorTableManager.h"
#include "Containers/Array.h"
#include "Platform/GenericPlatform.h"
#include "RenderResource.h"

namespace Lumina::RHI
{
    class FTextureManager
    {
    public:

        FTextureManager();
        
        void AddTexture(FRHIImage* InTexture);
		void RemoveTexture(const FRHIImage* InTexture);
        
        
        NODISCARD FRHIBindingLayout* GetLayout() const { return Layout; }
        NODISCARD FRHIDescriptorTable* GetDescriptorTable() const { return DescriptorTableManager.GetDescriptorTable(); }

    private:
        
        FSharedMutex                Mutex;
        FDescriptorTableManager     DescriptorTableManager;
        FRHIBindingLayoutRef        Layout;
        THashMap<FRHIImage*, int32> TextureMap;
    };
}
