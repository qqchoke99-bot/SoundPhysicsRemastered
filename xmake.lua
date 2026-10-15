add_rules("mode.release", "mode.debug")
-- OPENCODE: add_requires preloader like Resound; pin Levi 1.5.x
target("SoundPhysiczRecreate")
    set_kind("shared")
    set_languages("c++20")
    set_basename("SoundPhysiczRecreate")
    add_files("src/**.cpp")
    add_includedirs("include", "src")
    if is_plat("android") then
        set_runtimes("c++_static")
        add_cxflags("-fPIC", "-Oz", "-fno-rtti", "-fvisibility=hidden")
        add_shflags("-Wl,--gc-sections", "-flto", "-Wl,-z,max-page-size=16384")
        add_syslinks("log", "dl", "android", "m")
        -- Do NOT link libfmod — dlsym at runtime only (VCX approach)
    end
