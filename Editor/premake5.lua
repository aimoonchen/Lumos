IncludeDir = {}
IncludeDir["entt"] = "../Lumos/External/entt/src/"
IncludeDir["GLFW"] = "../Lumos/External/glfw/include/"
IncludeDir["Glad"] = "../Lumos/External/glad/include/"
IncludeDir["lua"] = "../Lumos/External/lua/src/"
IncludeDir["stb"] = "../Lumos/External/stb/"
IncludeDir["OpenAL"] = "../Lumos/External/OpenAL/include/"
IncludeDir["Box2D"] = "../Lumos/External/box2d/include/"
IncludeDir["vulkan"] = "../Lumos/External/vulkan/"
IncludeDir["Lumos"] = "../Lumos/Source"
IncludeDir["External"] = "../Lumos/External/"
IncludeDir["ImGui"] = "../Lumos/External/imgui/"
IncludeDir["freetype"] = "../Lumos/External/freetype/include"
IncludeDir["SpirvCross"] = "../Lumos/External/vulkan/SPIRV-Cross"
IncludeDir["cereal"] = "../Lumos/External/cereal/include"
IncludeDir["spdlog"] = "../Lumos/External/spdlog/include"
IncludeDir["glm"] = "../Lumos/External/glm"
IncludeDir["msdf_atlas_gen"] = "../Lumos/External/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "../Lumos/External/msdf-atlas-gen/msdfgen"

project "LumosEditor"
	kind "ConsoleApp"
	language "C++"
	editandcontinue "Off"
	
	files
	{
		"**.h",
		"**.cpp",
		"Source/**.h",
		"Source/**.cpp"
	}
	
	includedirs
	{
		"../Lumos/Source/Lumos",
	}

	externalincludedirs
	{
		"%{IncludeDir.entt}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.OpenAL}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.External}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.freetype}",
		"%{IncludeDir.SpirvCross}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Lumos}",
	}

	links
	{
		"Lumos",
		"lua",
		"box2d",
		"imgui",
		"freetype",
		"SpirvCross",
		"spdlog",
		"meshoptimizer",
		-- "msdfgen",
		"msdf-atlas-gen"
	}

	defines
	{
		"IMGUI_USER_CONFIG=\"../../Lumos/Source/Lumos/ImGui/ImConfig.h\"",
		"SPDLOG_COMPILED_LIB",
		"GLM_FORCE_INTRINSICS",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE"
	}

	filter { "files:External/**"}
		warnings "Off"

	filter 'architecture:x86_64'
		defines { "USE_VMA_ALLOCATOR"}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		conformancemode "off"

		defines
		{
			"LUMOS_PLATFORM_WINDOWS",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_EXTENDED_ALIGNED_STORAGE",
			"_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
			"LUMOS_VOLK"
		}

		libdirs
		{
			"../Lumos/External/OpenAL/libs/Win32"
		}

		links
		{
			"glfw",
			"OpenGL32",
			"OpenAL32"
		}

		postbuildcommands { "xcopy /Y /C \"..\\Lumos\\External\\OpenAL\\libs\\Win32\\OpenAL32.dll\" \"$(OutDir)\"" } 

		disablewarnings { 4307 }

		filter {'system:windows', 'configurations:Production'}
			kind "WindowedApp"

	filter "system:macosx"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "11.0"
		editandcontinue "Off"
		kind "WindowedApp"
		xcodebuildresources { "Assets.xcassets", "libMoltenVK.dylib" }

		xcodebuildsettings
		{
			['ARCHS'] = false,
			['CODE_SIGN_IDENTITY'] = '',
			['INFOPLIST_FILE'] = '../Lumos/Source/Lumos/Platform/macOS/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon'
			--['ENABLE_HARDENED_RUNTIME'] = 'YES'
		}

		if settings.enable_signing then
		xcodebuildsettings
		{
			['CODE_SIGN_IDENTITY'] = 'Mac Developer',
			['DEVELOPMENT_TEAM'] = settings.developer_team,
			['PRODUCT_BUNDLE_IDENTIFIER'] = settings.bundle_identifier
		}
		end

		defines
		{
			"LUMOS_PLATFORM_MACOS",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_EXT_metal_surface",
			"LUMOS_IMGUI",
			"LUMOS_VOLK"
		}

		linkoptions
		{
			"-framework OpenGL",
			"-framework Cocoa",
			"-framework IOKit",
			"-framework CoreVideo",
			"-framework OpenAL",
			"-framework QuartzCore"
		}

		files
		{
			"../Resources/AppIcons/Assets.xcassets",
			--"../Lumos/External/vulkan/libs/macOS/libMoltenVK.dylib"
		}

		links
		{
			"glfw",
		}

		SetRecommendedXcodeSettings()

	filter "system:ios"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		kind "WindowedApp"
		targetextension ".app"
		editandcontinue "Off"

		defines
		{
			"LUMOS_PLATFORM_IOS",
			"LUMOS_PLATFORM_MOBILE",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_IOS_MVK",
			"LUMOS_IMGUI"
		}

		links
		{
			"QuartzCore.framework",
			"Metal.framework",
            "MetalKit.framework",
        	"IOKit.framework",
        	"CoreFoundation.framework",
			"CoreVideo.framework",
			"CoreGraphics.framework",
			"UIKit.framework",
			"OpenAL.framework",
			"AudioToolbox.framework",
			"Foundation.framework",
			"SystemConfiguration.framework",
		}

		linkoptions
		{
			"../Lumos/External/vulkan/libs/iOS/libMoltenVK.a"
		}

		--Adding assets from Game Project folder. Needs rework
		files
		{
			"../Resources/AppIcons/Assets.xcassets",
			"../Lumos/Assets/Shaders",
			"../Lumos/Source/Lumos/Platform/iOS/Client/**",
            "../ExampleProject/Assets/Scenes",
			"../ExampleProject/Assets/Scripts",
			"../ExampleProject/Assets/Meshes",
			"../ExampleProject/Assets/Sounds",
			"../ExampleProject/Assets/Textures",
			"../ExampleProject/Example.lmproj"
		}

		xcodebuildsettings
		{
			['ARCHS'] = '$(ARCHS_STANDARD)',
			['ONLY_ACTIVE_ARCH'] = 'NO',
			['SDKROOT'] = 'iphoneos',
			['TARGETED_DEVICE_FAMILY'] = '1,2',
			['SUPPORTED_PLATFORMS'] = 'iphonesimulator iphoneos',
			['CODE_SIGN_IDENTITY'] = '',
			['IPHONEOS_DEPLOYMENT_TARGET'] = '13.0',
			['INFOPLIST_FILE'] = '../Lumos/Source/Lumos/Platform/iOS/Client/Info.plist',
			['ASSETCATALOG_COMPILER_APPICON_NAME'] = 'AppIcon',
}

if settings.enable_signing then
xcodebuildsettings
{
	['PRODUCT_BUNDLE_IDENTIFIER'] = settings.bundle_identifier,
	['CODE_SIGN_IDENTITY[sdk=iphoneos*]'] = 'iPhone Developer',
	['DEVELOPMENT_TEAM'] = settings.developer_team
}
end

		if _OPTIONS["teamid"] then
			xcodebuildsettings {
				["DEVELOPMENT_TEAM"] = _OPTIONS["teamid"]
			}
		end

		if _OPTIONS["tvOS"] then
			xcodebuildsettings {
				["SDKROOT"] = _OPTIONS["tvos"],
				['TARGETED_DEVICE_FAMILY'] = '3',
				['SUPPORTED_PLATFORMS'] = 'tvos'
			}
		end

		linkoptions { "-rpath @executable_path/Frameworks" }

		excludes
		{
			("**.DS_Store")
		}

		xcodebuildresources
		{
			"../Lumos/Source/Platform/iOS/Client",
			"Assets.xcassets",
            "Shaders",
            "Meshes",
            "Scenes",
            "Scripts",
            "Sounds",
			"Textures",
			"Example.lmproj"
		}

		SetRecommendedXcodeSettings()

	filter "system:linux"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"LUMOS_PLATFORM_LINUX",
			"LUMOS_PLATFORM_UNIX",
			"LUMOS_RENDER_API_OPENGL",
			"LUMOS_RENDER_API_VULKAN",
			"VK_USE_PLATFORM_XCB_KHR",
			"LUMOS_IMGUI",
			"LUMOS_VOLK"
		}

		buildoptions
		{
			"-fpermissive",
			"-Wattributes",
			"-fPIC",
			"-Wignored-attributes",
			"-Wno-psabi"
		}

		links
		{
			"glfw",
		}

		links { "X11", "pthread", "dl", "atomic", "stdc++fs", "openal"}

		linkoptions { "-L%{cfg.targetdir}", "-Wl,-rpath=\\$$ORIGIN"}

		filter {'system:linux', 'architecture:x86_64'}
			buildoptions
			{
				"-msse4.1",
			}

	filter "configurations:Debug"
		defines { "LUMOS_DEBUG", "_DEBUG","TRACY_ENABLE","LUMOS_PROFILE","TRACY_ON_DEMAND" }
		symbols "On"
		runtime "Debug"
		optimize "Off"

	filter "configurations:Release"
		defines { "LUMOS_RELEASE","TRACY_ENABLE","LUMOS_PROFILE","TRACY_ON_DEMAND"}
		optimize "Speed"
		symbols "On"
		runtime "Release"

	filter "configurations:Production"
		defines "LUMOS_PRODUCTION"
		symbols "Off"
		optimize "Full"
		runtime "Release"
