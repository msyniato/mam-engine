project "01_window"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access

	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
		"../mam/include",
		"../mam/vendors/stb_image",
		"../mam/vendors/stb_vorbis",
		"../mam/vendors/openal_soft/include",
		"../mam/vendors/imgui",
		"../mam/vendors/imgui/backends",
		"../mam/vendors/dr",
		"../mam/vendors/lua/src",
		"../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)

	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/01_window/data" }
	
	files "01_window/01_window.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "02_triangle"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access

	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib  
	dependson "mam_engine"
	use_vulkan_sdk(true)

	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/02_triangle/data" }
	
	files "02_triangle/02_triangle.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "03_moving_triangle"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/03_moving_triangle/data" }
	
	files "03_moving_triangle/03_moving_triangle.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "04_mesh"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/04_mesh/data" }
	
	files "04_mesh/04_mesh.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "05_job_system"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/05_job_system/data" }
	
	files "05_job_system/05_job_system.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "06_ecs"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/06_ecs/data" }
	
	files "06_ecs/06_ecs.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "07_scripting"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
		"../mam/include",
		"../mam/vendors/stb_image",
		"../mam/vendors/stb_vorbis",
		"../mam/vendors/openal_soft/include",
		"../mam/vendors/imgui",
		"../mam/vendors/imgui/backends",
		"../mam/vendors/dr",
		"../mam/vendors/lua/src",
		"../mam/vendors/sol2/include"
	}

    libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/07_scripting/data" }
	
	files "07_scripting/07_scripting.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "08_terrain"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
		"../mam/include",
		"../mam/vendors/stb_image",
		"../mam/vendors/stb_vorbis",
		"../mam/vendors/openal_soft/include",
		"../mam/vendors/imgui",
		"../mam/vendors/imgui/backends",
		"../mam/vendors/dr",
		"../mam/vendors/lua/src",
		"../mam/vendors/sol2/include",
		"../mam/vendors/Perlin/include/Perlin/",
		"../mam/vendors/Perlin/src"
	}

    libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)

	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/08_terrain/data" }
	
	files "08_terrain/08_terrain.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "09_light_probe"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/09_light_probe/data" }
	
	files "09_light_probe/09_light_probe.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "10_deferred"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/10_deferred/data" }
	
	files "10_deferred/10_deferred.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "11_shadow"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/11_shadow/data" }
	
	files "11_shadow/11_shadow.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}


project "95_audio"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/95_audio/data" }
	
	files "95_audio/95_audio.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "96_branching"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/96_branching/data" }
	
	files "96_branching/96_branching.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "97_audio_demo"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/97_audio_demo/data" }
	
	files "97_audio_demo/97_audio_demo.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "98_vulkan_sandbox"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)

	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/98_vulkan_sandbox/data" }
	
	files "98_vulkan_sandbox/98_vulkan_sandbox.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

project "99_sandbox"
	
	kind "ConsoleApp"  -- Change to "WindowedApp" to remove console access
	
	language "C++"
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	
	includedirs {
        "../mam/include",
        "../mam/vendors/stb_image",
        "../mam/vendors/stb_vorbis",
        "../mam/vendors/openal_soft/include",
        "../mam/vendors/imgui",
        "../mam/vendors/imgui/backends",
        "../mam/vendors/dr",
        "../mam/vendors/lua/src",
        "../mam/vendors/sol2/include"
	}
	
	libdirs { "../mam/vendors/openal_soft/libs/Win64" }
	links { "mam_engine", "OpenAL32" } -- links engine .lib 
	dependson "mam_engine"
	use_vulkan_sdk(true)
	
	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/99_sandbox/data" }
	
	files "99_sandbox/99_sandbox.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}