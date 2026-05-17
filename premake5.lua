project("chronology.v1")
    kind("StaticLib")
    language("C")
    targetdir("tests/")
    objdir("build/obj/${cfg.buildcfg}")
    location("build")

    files({
        "include/chronology.h",
        "include/conf.h",
        "src/chronology.c"
    })

    includedirs({
        "include"
    })

    filter("configurations:*debug* or *Debug*")
        symbols("on")

    filter("configurations:*release* or *Release*")
        symbols("off")
        optimize("full")
    
project("chronology_lua.v1")
    kind("SharedLib")
    language("C")
    targetdir("tests")
    objdir("build/obj/${cfg.buildcfg}")
    location("build")
    targetprefix("")
    targetname("chronology_lua")
    pic("on")

    files({
        "include/**.h",
        "src/**.c"
    })

    includedirs({
        "include"
    })

    filter("configurations:*debug* or *Debug*")
        symbols("on")

    filter("configurations:*release* or *Release*")
        symbols("off")
        optimize("full")
