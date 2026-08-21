-- ============================================================
--  MAMEngine SDK - premake5.lua
--  Generado automaticamente por genStage.bat
--  Ejecuta: premake5 vs2022
-- ============================================================

conan = {}
configs = { "Debug", "Release", "RelWithDebInfo" }

for i = 1, 3 do
    include("build/deps/" .. configs[i] .. "/conanbuildinfo.premake.lua")
    conan[configs[i]] = {}
    local cfg = conan[configs[i]]
    cfg["build_type"]       = conan_build_type
    cfg["arch"]             = conan_arch
    cfg["includedirs"]      = conan_includedirs
    cfg["libdirs"]          = conan_libdirs
    cfg["bindirs"]          = conan_bindirs
    cfg["libs"]             = conan_libs
    cfg["system_libs"]      = conan_system_libs
    cfg["defines"]          = conan_defines
    cfg["cxxflags"]         = conan_cxxflags
    cfg["cflags"]           = conan_cflags
    cfg["sharedlinkflags"]  = conan_sharedlinkflags
    cfg["exelinkflags"]     = conan_exelinkflags
    cfg["frameworks"]       = conan_frameworks
end

-- Configura include/link de conan para un ejecutable
function conan_config_exec()
    local cfgs = { "Debug", "Release", "RelWithDebInfo" }
    for i = 1, 3 do
        local cfg = conan[cfgs[i]]
        filter("configurations:" .. cfgs[i])
            linkoptions { cfg["exelinkflags"] }
            includedirs { cfg["includedirs"] }
            libdirs     { cfg["libdirs"] }
            links       { cfg["libs"] }
            links       { cfg["system_libs"] }
            links       { cfg["frameworks"] }
            defines     { cfg["defines"] }
        filter {}
    end
end

-- Configura include/link del motor segun configuracion
function engine_link()
    filter "configurations:Debug"
        libdirs { "lib/Debug" }
        links   { "mam_engine" }
    filter "configurations:Release"
        libdirs { "lib/Release" }
        links   { "mam_engine" }
    filter "configurations:RelWithDebInfo"
        libdirs { "lib/RelWithDebInfo" }
        links   { "mam_engine" }
    filter {}
end

-- ============================================================
--  Workspace
-- ============================================================
workspace "MAMEngineSDK"
    configurations { "Debug", "Release", "RelWithDebInfo" }
    architecture "x64"
    location "build"
    cppdialect "c++20"
    startproject "01_window"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        runtime "Release"

    filter "configurations:RelWithDebInfo"
        defines { "NDEBUG" }
        optimize "On"
        runtime "Release"
        symbols "On"

    filter {}

-- ============================================================
--  Helper: crea un proyecto de ejemplo con toda la config
-- ============================================================
function example_project(name, src_file)
    project(name)
        kind "ConsoleApp"
        language "C++"
        targetdir ("build/" .. name .. "/%{cfg.buildcfg}")

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

	conan_config_exec("Debug")
	conan_config_exec("Release")
	conan_config_exec("RelWithDebInfo")
	
	debugargs { _MAIN_SCRIPT_DIR .. "/01_window/data" }
	
	files "game/game.cpp"
	
	postbuildcommands {
		"call ..\\tools\\copy_assets.bat"
	}

end

-- ============================================================
--  Proyectos de ejemplo
-- ============================================================
group "Examples"
    example_project("01_window",          "01_window/01_window.cpp")
    example_project("02_triangle",        "02_triangle/02_triangle.cpp")
    example_project("03_moving_triangle", "03_moving_triangle/03_moving_triangle.cpp")
    example_project("04_mesh",            "04_mesh/04_mesh.cpp")
    example_project("05_job_system",      "05_job_system/05_job_system.cpp")
    example_project("06_ecs",             "06_ecs/06_ecs.cpp")
    example_project("07_scripting",       "07_scripting/07_Scripting.cpp")
group ""
