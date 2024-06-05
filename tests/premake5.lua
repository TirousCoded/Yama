

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
		"../yama",
		"../vendor/googletest/googletest/include",
		"../vendor/googletest/googletest/"
	}
	
	links
	{
		"YamaLib"
	}

