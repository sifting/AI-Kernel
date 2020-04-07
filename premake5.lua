workspace "AI"
	configurations {"Debug", "Release"}
	location "build"
	targetdir "."
	debugdir "."
	filter "language:C"
		toolset "gcc"
		buildoptions {"-std=c11 -pedantic -Wall"}
	filter "system:windows"
		links {"mingw32"}
	filter "configurations:Debug"
		defines {"DEBUG"}
		symbols "On"
	filter "configurations:Test"
		defines {"DEBUG", "TEST"}
		symbols "On"
	filter "configurations:Release"
		defines {"NDEBUG"}
		vectorextensions "Default"
		optimize "Speed"
	--Kernel
	project "kernel"
		postbuildcommands {"cd .. && python copy.py src ai"}
		includedirs "src/public"
		language "C"
		kind "StaticLib"
		files {
			"src/**.h",
			"src/**.c",
		}
		removefiles "src/main.c"
		filter {}
	--Tests
	project "test"
		language "C"
		kind "ConsoleApp"
		includedirs "include"
		files "src/main.c"
		links {"kernel"}
		filter {}
