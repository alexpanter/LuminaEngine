project "SLang"
	kind "Utility"
	warnings "off"

	files
	{
		"**.h",
		"**.lua",
	}

	postbuildcommands 
	{ 
		LuminaConfig.MakeDirectory(LuminaConfig.GetTargetDirectory()),
		LuminaConfig.CopyFile(LuminaConfig.EnginePath("External/SLang/bin/slang.dll"), LuminaConfig.GetTargetDirectory()),
		LuminaConfig.CopyFile(LuminaConfig.EnginePath("External/SLang/bin/slang-compiler.dll"), LuminaConfig.GetTargetDirectory()),
	}