


project "mam_engine"
	
	kind "StaticLib"  -- Staticlib --> .lib / SharedLib --> .dll
	
	targetdir "../build/%{prj.name}/%{cfg.buildcfg}"
	targetname "mam_engine"
	
	includedirs {"include/mam"}
	externalincludedirs { 
		"vendors/stb_image", 
		"vendors/openal_soft/include", 
		"vendors/imgui", 
		"vendors/imgui/backends",
		"vendors/stb_vorbis",
		"vendors/dr", 
		"vendors/sol2/include", 
		"vendors/lua/src", 
		"vendors/Perlin/include"}

	conan_config_lib()
	
	files {
		-- premake files
		"premake5_engine.lua",
		
		-- conan files
		"src/build/conanfile.txt",
		
		-- header files
		"include/mam/**.hpp",
		"include/mam/common/**.hpp",
		"include/mam/audiosys/**.hpp",
		"include/mam/ecs/**.hpp",
		"include/mam/core/**.hpp",
		"include/mam/jobsys/**.hpp",
		"include/mam/matsys/**.hpp",		
		"include/mam/render/api/**.hpp",		
		"include/mam/render/opengl/**.hpp",
		"include/mam/render/vulkan/**.hpp",
		"include/mam/scriptsys/**.hpp",
		"include/mam/ui/**.hpp",

		-- source files
		"src/audiosys/**.cpp",
		"src/ecs/**.cpp",
		"src/core/**.cpp",
		"src/jobsys/**.cpp",
		"src/matsys/**.cpp",		
		"src/render/api/**.cpp",		
		"src/render/opengl/**.cpp",
		"src/render/vulkan/**.cpp",
		"src/scriptsys/**.cpp",
		"src/ui/**.cpp",

		-- shader files
		"../assets/shaders/**",

        "../assets/script/**.lua",
        "../assets/textures/**",
		
		-- vendors
		"vendors/imgui/backends/imgui_impl_glfw.cpp", "vendors/imgui/backends/imgui_impl_opengl3.cpp",
		"vendors/imgui/*.cpp",
		"vendors/imgui/backends/imgui_impl_vulkan.cpp",
		"vendors/stb_image/*.cpp",
		"vendors/lua/src/*.c", 
		"vendors/sol2/include/sol/**.hpp",
		"vendors/Perlin/src/**.cpp"
		-- "vendors/stb_vorbis/*.c"
	}

	vpaths {
		["shaders/*"] = { "../assets/shaders/**" }
	}

    removefiles {
        "vendors/lua/src/lua.c",
        "vendors/lua/src/luac.c"
    }

	use_vulkan_sdk(false)