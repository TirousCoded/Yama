

project "YamaLib"
	
	kind "StaticLib"
	targetname "yamalib"
	
	files
	{
		"**.h",
		"**.cpp",
		"**.c",
		"**.txt",
		"**.taul",
		"**.yama"
	}
	
	includedirs
	{
        "../vendor/TAUL/taul"
	}
	
	links
	{
		"TAUL"
	}

