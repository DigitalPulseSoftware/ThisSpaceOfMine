add_requires("catch2 >=3.x")

target("UnitTests", function ()
    if has_config("asan") then
        add_defines("CATCH_CONFIG_NO_WINDOWS_SEH")
        add_defines("CATCH_CONFIG_NO_POSIX_SIGNALS")
    end

    add_deps("CommonLib")
    add_packages("catch2")
    add_files("**.cpp")
end)
