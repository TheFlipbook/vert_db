solution "vert_db"
    configurations { "Debug", "Release" }
    platforms { "x86", "x64" }
    location "./_build"
    characterset 'Unicode'

    startproject 'vert_db-test'

    defines {
        "_WINDOWS",
        "USE_FUZZY_VECTOR_EQUAL_OPERATORS",
    }

    libdirs {
    }

    configuration "Debug"
        defines { "DEBUG" }
        symbols 'on'

    configuration "Release"
        defines { "NDEBUG" }
        optimize  'On'

    configuration "x32"
        defines { "_WIN32" }

    configuration "x64"
        defines { "_WIN64" }


    project "vert_db"
        kind "StaticLib"
        language "C++"
        location "./_build/projects"
        targetdir "./_bin/%{cfg.buildcfg}/%{cfg.platform}/bin/"
        implibdir "./_bin/%{cfg.buildcfg}/%{cfg.platform}/lib/"

        includedirs {
            "./include/",
        }

        files {
            "./src/**.h",
            "./src/**.cpp",
            "./include/**.h",
        }

    project "vert_db-test"
        kind "ConsoleApp"
        language "C++"
        location "./_build/projects"
        targetdir "./_bin/%{cfg.buildcfg}/%{cfg.platform}/bin/"
        implibdir "./_bin/%{cfg.buildcfg}/%{cfg.platform}/lib/"

        includedirs {
            "./include/",
            "./external/",
        }

        files {
            "./test/**.h",
            "./test/**.cpp",
        }

        dependson {
            "vert_db"
        }
