

project "Sandbox"
	
	kind "None" --"ConsoleApp"
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

