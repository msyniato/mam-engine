


-- Array/list that stores 3 build configurations and their respective variables.
conan = {}

-- List of build configurations
configs = { 'Debug', 'Release', 'RelWithDebInfo' }

-- Loop that fills conan array
for i = 1, 3 do
	-- Include .lua file that conan generated for a specific build configuration
	include("build/deps/" .. configs[i] .. "/conanbuildinfo.premake.lua")

	-- Creation of dictionary for a specific configuration
	conan[configs[i]] = {}
	local cfg = conan[configs[i]]

	-- Storing variables generated for a specific configuration in respective dictionary
	cfg["build_type"] = conan_build_type
	cfg["arch"] = conan_arch
	cfg["includedirs"] = conan_includedirs
	cfg["libdirs"] = conan_libdirs
	cfg["bindirs"] = conan_bindirs
	cfg["libs"] = conan_libs
	cfg["system_libs"] = conan_system_libs
	cfg["defines"] = conan_defines
	cfg["cxxflags"] = conan_cxxflags
	cfg["cflags"] = conan_cflags
	cfg["sharedlinkflags"] = conan_sharedlinkflags
	cfg["exelinkflags"] = conan_exelinkflags
	cfg["frameworks"] = conan_frameworks
end

-- Function that configures .exe file for a specific build configuration
function conan_config_exec()
	-- List of build configurations
	configs = { 'Debug', 'Release', 'RelWithDebInfo' }

	for i = 1, 3 do
		-- Dictionary for a specific configuration
		local cfg = conan[configs[i]]

		--[[
			Add a filter --> if configs[i] == configuration passed as parameter
			If filter is true than do the following functions
		]]
		filter("configurations:" .. configs[i])

			-- Apply build variables of configuration passed as parameter
			linkoptions { cfg["exelinkflags"] }
			includedirs { cfg["includedirs"] }
			libdirs { cfg["libdirs"] }
			links { cfg["libs"] }
			links { cfg["system_libs"] }
			links { cfg["frameworks"] }
			defines { cfg["defines"] }

		-- Reset filter
		filter {}
	end
end

-- Function that configures .lib file for a specific build configuration
function conan_config_lib()
	-- List of build configurations
	configs = { 'Debug', 'Release', 'RelWithDebInfo' }

	for i = 1, 3 do
		-- Dictionary for a specific configuration
		local cfg = conan[configs[i]]

		--[[
			Add a filter --> if configs[i] == configuration passed as parameter
			If filter is true than do the following functions
		]]
		filter("configurations:" .. configs[i])

			-- Apply build variables of configuration passed as parameter
			linkoptions { cfg["sharedlinkflags"] }
			includedirs { cfg["includedirs"] }
			defines { cfg["defines"] }

		-- Reset filter
		filter {}
	end
end

local VULKAN_SDK = os.getenv("VULKAN_SDK")

function use_vulkan_sdk(copy_runtime_dll)
    if VULKAN_SDK ~= nil then
        includedirs {
            VULKAN_SDK .. "/Include"
        }

        libdirs {
            VULKAN_SDK .. "/Lib"
        }

        links {
            "vulkan-1",
            "shaderc_shared"
        }

        defines {
            "SHADERC_SHAREDLIB"
        }

        if copy_runtime_dll then
            postbuildcommands {
                '{COPY} "' .. VULKAN_SDK .. '/Bin/shaderc_shared.dll" "%{cfg.targetdir}"'
            }
        end

       
    else
        print("WARNING: Vulkan SDK not found! Install Vulkan SDK and restart your PC.")
    end
end


workspace "MAMEngine"
	configurations { "Debug", "Release", "RelWithDebInfo" }
	architecture "x64"
	location "build"
	cppdialect "c++20"
	startproject "mam_engine"

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

	-- Include premake5_root.lua file to workspace directly (???)

-- Include all directories that have their own .lua files
group "Engine"
    include "mam/premake5_engine"

group "Workspaces" -- In this case we may consider adding separate .lua file in the future for each workspace
    include "workspaces/premake5_workspaces"

group ""