#pragma once

#include "Containers/Name.h"
#include "Renderer/ErrorHandling/CrashTracker.h"
#include "VkBootstrap.h"

namespace Lumina::RHI
{
    class FVulkanCrashTracker : public ICrashTracker
    {
    public:
        
        FVulkanCrashTracker();
        ~FVulkanCrashTracker() override = default;
        LE_NO_COPYMOVE(FVulkanCrashTracker);
        
        void Initialize(RHIDevice InDevice, RHIPhysicalDevice InPhysicalDevice) override;
        void Shutdown() override;

        void OnDeviceLost() override;
        
        // Call after device creation to enable Aftermath features
        void EnableDeviceFeatures(vkb::DeviceBuilder& Builder);
        
        void RegisterShader(const TVector<uint32>& SPRIV, const FString& Name) override;
        void SetMarker(RHICommandBuffer cmdBuffer, const char* markerName) override;
        void BeginMarker(RHICommandBuffer cmdBuffer, const char* markerName) override;
        void EndMarker(RHICommandBuffer cmdBuffer) override;
        void PollCrashDumps() override;
        
        const FString& GetCrashDumpDirectory() const { return CrashDumpDirectory; }
        
    private:
        
        VkDevice Device = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        
        FString CrashDumpDirectory;
    };
    
}
