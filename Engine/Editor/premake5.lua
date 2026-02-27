project "Editor"
	kind "SharedLib"
    rtti "off"
	enablereflection "true"
	removeplatforms { "Game" }

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
		
        "Luau.Compiler",
        "Luau.Config",
        "Luau.VM",
        "Luau.AST",
        "Luau.Common",

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