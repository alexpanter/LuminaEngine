project "BasicUniversal"
	kind "StaticLib"
	warnings "off"
    

	defines
	{
		"BASISD_SUPPORT_KTX2",
		"BASISD_SUPPORT_KTX2_ZSTD=0",
	}

	files
	{
		"**.h",
		"**.cpp",
		"**.lua",
	}