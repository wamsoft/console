--
-- Premake 4.x build configuration script
--

build_base = "./"
project_name = "krkrconsole"

	solution (project_name)
		configurations { "Release", "Debug" }
		location   (build_base .. _ACTION)
		objdir     (build_base .. _ACTION .. "/obj")
		targetdir  ("bin")
		flags
		{
			"No64BitChecks",
--			"ExtraWarnings",
			"StaticRuntime",	-- set /MT(d) option.

			"NoManifest",		-- Prevent the generation of a manifest.
			"NoMinimalRebuild",	-- Disable minimal rebuild.
--			"NoIncrementalLink",	-- Disable incremental linking.
			"NoPCH",		-- Disable precompiled header support.
		}

		includedirs
		{
			"wineditline/src",
			"..",
			"../ncbind",
			"./",
		}

		configuration "Debug"
			defines     "_DEBUG"
			symbols		"On"
			targetsuffix "-d"

		configuration "Release"
			defines     "NDEBUG"
			optimize	"On"

	project (project_name)
		targetname  (project_name)
		language    "C++"
		kind        "SharedLib"

		files
		{
			"wineditline/src/editline.c",
			"wineditline/src/history.c",
			"wineditline/src/fn_complete.c",
			"../tp_stub.cpp",
			"../ncbind/ncbind.cpp",
			"main.cpp",
		}
