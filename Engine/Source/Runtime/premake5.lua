project "Runtime"
    kind "SharedLib"
    rtti "off"
    staticruntime "Off"
    pchheader "pch.h"
    pchsource "pch.cpp"
    dependson { "Reflector" }
    enablereflection "true"

    defines
    {
        "EASTL_USER_DEFINED_ALLOCATOR=1",
        "GLFW_INCLUDE_NONE",
        "GLFW_STATIC",
        "LUMINA_RENDERER_VULKAN",
        "VK_NO_PROTOTYPES",
        "LUMINA_RPMALLOC",
        --"WITH_AFTERMATH",
    }

    linkoptions
    {
        "/NODEFAULTLIB:LIBCMT"
    }

    files
    {
        "**.cpp",
        "**.h",
        "**.lua",
        "**.slang",
        LuminaConfig.GetReflectionFiles()
    }

    prebuildcommands 
    {
        LuminaConfig.CopyFile(LuminaConfig.EnginePath("Engine/Source/ThirdParty/NvidiaAftermath/lib/GFSDK_Aftermath_Lib.x64.dll"), LuminaConfig.GetTargetDirectory()),
        LuminaConfig.CopyFile(LuminaConfig.EnginePath("Engine/Source/ThirdParty/NvidiaAftermath/lib/GFSDK_Aftermath_Lib.lib"), LuminaConfig.GetTargetDirectory()),
        LuminaConfig.RunReflection()
    }

    includedirs
    {
        LuminaConfig.GetPublicIncludeDirectories()
    }

    filter "configurations:Debug"
	libdirs 
	{ 
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/luau/libs/Debug"),
	}
	filter {}

	filter "configurations:Development or Shipping"
	libdirs
	{
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/luau/libs/Release"),
	}
	filter {}
    
    libdirs
    {
        LuminaConfig.EnginePath("Engine/Source/ThirdParty/NvidiaAftermath/lib"),
        LuminaConfig.EnginePath("External/SLang/lib"),
    }

    fatalwarnings
    {
        "4456",
        "4457",
        "4458",
        "4238",
    }

    forceincludes
	{
		"LuminaAPI.h",
	}

    links
    {
        "MiniAudio",
        "GLFW",
        "ImGui",
        "EA",
        "Tracy",

        "Luau.Analysis",
        "Luau.Compiler",
        "Luau.Config",
        "Luau.VM",
        "Luau.AST",
        "Luau.Common",

        "EnkiTS",
        "JoltPhysics",
        "RPMalloc",
        "XXHash",
        "Volk",
        "VKBootstrap",
        "TinyOBJLoader",
        "MeshOptimizer",
        "SPIRV-Reflect",
        "FastGLTF",
        "OpenFBX",
        "BasicUniversal",
        "slang",
        "slang-compiler",
        "GFSDK_Aftermath_Lib.lib",
        "GFSDK_Aftermath_Lib.x64.dll",
    }        
        
    filter "files:Engine/Source/ThirdParty/**.cpp"
        enablepch "Off"
    filter {}

    filter "files:Engine/Source/ThirdParty/**.c"
        enablepch "Off"
        language "C"
    filter {}