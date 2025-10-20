

project "Yama.Tests"
	
	kind "ConsoleApp"
	targetname "Tests"
	
	files
	{
		"**.h",
		"**.cpp",
		"**.c",
		"**.txt",
		"**.taul",
		"**.yama",
        "../vendor/googletest/googletest/src/gtest-all.cc"
	}
	
	includedirs
	{
		"../Yama",
        "../vendor/TAUL/taul",
		"../vendor/googletest/googletest/include",
		"../vendor/googletest/googletest/"
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

