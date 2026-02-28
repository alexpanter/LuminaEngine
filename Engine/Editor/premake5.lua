project "Editor"
	kind "SharedLib"
    rtti "off"
	enablereflection "true"
	removeplatforms { "Game" }

    libdirs
    {
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/NvidiaAftermath/lib"),
    }

	linkoptions
    {
        "/NODEFAULTLIB:LIBCMT"
    }

	prebuildcommands
	{
		LuminaConfig.RunReflection()
	}

	defines
	{
		"RUNTIME_API=DLL_IMPORT"
	}

	links
	{
		"Runtime",
		"ImGui",
		"RPMalloc",
    	"EA",
		"EnkiTS",
		"Tracy",
		
        "Luau",

		"GFSDK_Aftermath_Lib",
	}

	files
	{
		"Source/**.h",
		"Source/**.cpp",
		"**.lua",
		LuminaConfig.GetReflectionFiles()
	}

	forceincludes
	{
		"EditorAPI.h",
	}

	includedirs
	{
        "Source",
		LuminaConfig.GetPublicIncludeDirectories(),
	}