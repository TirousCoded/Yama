

project "Sandbox"
	
	kind "ConsoleApp"
	targetname "sandbox"
	
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
		"../yamalib",
        "../vendor/TAUL/taul"
	}
	
	links
	{
		"YamaLib",
		"TAUL"
	}

