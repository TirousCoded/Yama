

project "Yama"
	
	kind "StaticLib"
	targetname "Yama"
	
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

