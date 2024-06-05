

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
		"../yama"
	}
	
	links
	{
		"YamaLib"
	}

