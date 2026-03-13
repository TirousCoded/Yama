

project "Yama.Sandbox"
	
	kind "ConsoleApp"
	targetname "Sandbox"
	
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
		"../Yama",
        "../vendor/TAUL/taul"
	}
	
	links
	{
		"Yama",
		"TAUL"
	}

	buildoptions
	{
		"/bigobj" -- Fat source files were causing C1128.
	}

