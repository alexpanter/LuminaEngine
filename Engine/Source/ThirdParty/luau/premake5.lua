project "Luau"
	kind "SharedLib"
	warnings "off"

	defines
	{
		"LUA_BUILD_AS_DLL",
		"LUA_LIB",
	}

	files
	{
		"include/**.h",
		"Source/**.cpp",
		"**.lua",
	}

	includedirs
	{
		"include"
	}