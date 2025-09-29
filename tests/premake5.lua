

project "Tests"
	
	kind "ConsoleApp"
	targetname "tests"
	
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
		"../yamalib",
        "../vendor/TAUL/taul",
		"../vendor/googletest/googletest/include",
		"../vendor/googletest/googletest/"
	}
	
	links
	{
		"YamaLib",
		"TAUL"
	}

	buildoptions
	{
		"/bigobj" -- Fat source files were causing C1128.
	}

