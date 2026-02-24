#include "pch.h"
#include "VulkanCrashTracker.h"
#include <fstream>
#include <NvidiaAftermath/GFSDK_Aftermath_GpuCrashDump.h>
#include "Log/Log.h"
#include "NvidiaAftermath/GFSDK_Aftermath.h"
#include "NvidiaAftermath/GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"

namespace Lumina::RHI
{
    static FString AftermathErrorMessage(GFSDK_Aftermath_Result Result)
    {
        switch (Result)
        {
        case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
            return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
        default:
            return "Aftermath Error 0x" + eastl::to_string(Result);
        }
    }
    
#ifdef _WIN32
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        MessageBoxA(0, AftermathErrorMessage(_result).c_str(), "Aftermath Error", MB_OK);               \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#else
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        printf("%s\n", AftermathErrorMessage(_result).c_str());                                         \
        fflush(stdout);                                                                                 \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#endif
    
    
    static FSharedMutex ShaderMutex;
    
    static void GpuCrashDumpCallback(const void* GpuCrashDump, uint32 GpuCrashDumpSize, void* UserData)
    {
        FVulkanCrashTracker* Tracker = static_cast<FVulkanCrashTracker*>(UserData);
        if (!Tracker)
        {
            return;
        }
    
        LOG_ERROR("Aftermath: GPU crash dump received ({} bytes) - decoding...", GpuCrashDumpSize);
        
        auto Now  = std::chrono::system_clock::now();
        auto Time = std::chrono::system_clock::to_time_t(Now);
    
        FString DumpPath = Tracker->GetCrashDumpDirectory() + "/GPUCrash_" + eastl::to_string(static_cast<uint64>(Time)) + ".nv-gpudmp";
        FString JsonPath = Tracker->GetCrashDumpDirectory() + "/GPUCrash_" + eastl::to_string(static_cast<uint64>(Time)) + ".json";
    
        {
            std::ofstream DumpFile(DumpPath.c_str(), std::ios::binary);
            if (DumpFile.is_open())
            {
                DumpFile.write(static_cast<const char*>(GpuCrashDump), GpuCrashDumpSize);
                LOG_INFO("Aftermath: Raw dump written to '{}'", DumpPath);
            }
        }
        
        GFSDK_Aftermath_GpuCrashDump_Decoder Decoder = {};
        GFSDK_Aftermath_Result DecodeResult = GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
            GFSDK_Aftermath_Version_API,
            GpuCrashDump,
            GpuCrashDumpSize,
            &Decoder);
    
        if (!GFSDK_Aftermath_SUCCEED(DecodeResult))
        {
            LOG_ERROR("Aftermath: Failed to create decoder");
            return;
        }
        
        uint32 JsonSize = 0;
        GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
            Decoder,
            GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
            GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
            nullptr, nullptr, nullptr,
            UserData,
            &JsonSize);
    
        if (JsonSize > 0)
        {
            TVector<char> Json(JsonSize);
            GFSDK_Aftermath_GpuCrashDump_GetJSON(Decoder, JsonSize, Json.data());
    
            std::ofstream JsonFile(JsonPath.c_str());
            if (JsonFile.is_open())
            {
                JsonFile.write(Json.data(), JsonSize);
                LOG_INFO("Aftermath: Full JSON dump written to '{}' - open this for complete crash details", JsonPath);
            }
        }
    
        GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(Decoder);
    }

    static void ShaderDebugInfoCallback(const void* ShaderDebugInfo, uint32 ShaderDebugInfoSize, void* UserData)
    {
        FWriteScopeLock Lock(ShaderMutex);
        
        GFSDK_Aftermath_ShaderDebugInfoIdentifier Identifier = {};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, ShaderDebugInfo, ShaderDebugInfoSize, &Identifier));
        
        
    }
    
    static void CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription AddDescription, void* UserData)
    {
        
    }

    static void ResolveMarkerCallback(const void* MarkerData, uint32 MarkerDataSize, void* UserData, PFN_GFSDK_Aftermath_ResolveMarker ResolveMarker)
    {
        
    }

    FVulkanCrashTracker::FVulkanCrashTracker()
    {
        CrashDumpDirectory = Paths::Combine(Paths::GetEngineInstallDirectory(), "CrashDumps");
        std::filesystem::create_directories(CrashDumpDirectory.c_str());
        
        #if WITH_AFTERMATH
        GFSDK_Aftermath_Result Result = GFSDK_Aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
            GpuCrashDumpCallback,
            ShaderDebugInfoCallback,
            CrashDumpDescriptionCallback,
            ResolveMarkerCallback,
            this
        );
        
        if (Result != GFSDK_Aftermath_Result_Success)
        {
            LOG_ERROR("Failed to initialize Nvidia Aftermath: {}", static_cast<int>(Result));
            return;
        }
        
        LOG_INFO("Nvidia Aftermath crash tracker initialized (Vulkan)");
        #endif
    }
    
    void FVulkanCrashTracker::Initialize(RHIDevice InDevice, RHIPhysicalDevice InPhysicalDevice)
    {
        Device = static_cast<VkDevice>(InDevice);
        PhysicalDevice = static_cast<VkPhysicalDevice>(InPhysicalDevice);
    }

    void FVulkanCrashTracker::Shutdown()
    {
        #if WITH_AFTERMATH
        GFSDK_Aftermath_DisableGpuCrashDumps();
        
        Device = VK_NULL_HANDLE;
        PhysicalDevice = VK_NULL_HANDLE;
        
        LOG_INFO("Nvidia Aftermath crash tracker shut down");
        #endif
    }

    void FVulkanCrashTracker::OnDeviceLost()
    {
        #if WITH_AFTERMATH
        GFSDK_Aftermath_CrashDump_Status Status = GFSDK_Aftermath_CrashDump_Status_Unknown;
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&Status));

        auto TerminationTimeout = std::chrono::seconds(3);
        auto tStart = std::chrono::steady_clock::now();
        auto tElapsed = std::chrono::milliseconds::zero();

        // Loop while Aftermath crash dump data collection has not finished or
        // the application is still processing the crash dump data.
        while (Status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed && Status != GFSDK_Aftermath_CrashDump_Status_Finished && tElapsed < TerminationTimeout)
        {
            // Sleep a couple of milliseconds and poll the status again.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&Status));

            tElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tStart);
        }

        if (Status == GFSDK_Aftermath_CrashDump_Status_Finished)
        {
            LOG_INFO("Aftermath finished processing crash dump");
        }
        else
        {
            LOG_ERROR("Unexpected crash dump status");
        }
        #endif
        
        PANIC("Vulkan detected a crash");
    }

    void FVulkanCrashTracker::EnableDeviceFeatures(vkb::DeviceBuilder& Builder)
    {
        #if WITH_AFTERMATH
        VkDeviceDiagnosticsConfigCreateInfoNV DiagnosticsConfig = {};
        DiagnosticsConfig.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
        DiagnosticsConfig.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV
                                | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV
                                | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV
                                | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV;

        
        Builder.add_pNext(&DiagnosticsConfig);
        #endif
    }

    void FVulkanCrashTracker::RegisterShader(const TVector<uint32>& SPRIV, const FString& Name)
    {

    }

    void FVulkanCrashTracker::SetMarker(RHICommandBuffer cmdBuffer, const char* markerName)
    {
    }

    void FVulkanCrashTracker::BeginMarker(RHICommandBuffer cmdBuffer, const char* markerName)
    {
    }

    void FVulkanCrashTracker::EndMarker(RHICommandBuffer cmdBuffer)
    {
    }

    void FVulkanCrashTracker::PollCrashDumps()
    {
    }
}
