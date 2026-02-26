project "JoltPhysics"
	kind "StaticLib"
	warnings "off"

	filter "configurations:Debug"
		defines 
		{
			"JPH_DEBUG_RENDERER",
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
			"JPH_EXTERNAL_PROFILE",
			"JPH_ENABLE_ASSERTS",
		}
	filter {}

	files
	{
		"**.h",
		"**.cpp",
		"**.lua",
	}

	includedirs
	{
		".",
	}