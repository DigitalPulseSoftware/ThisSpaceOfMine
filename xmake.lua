add_repositories("nazara-repo https://github.com/NazaraEngine/xmake-repo.git")
add_requires("nazaraengine", { configs = { debug = is_mode("debug") }})

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

set_project("ThisSpaceOfMine")

set_languages("c++20")
set_rundir(".")
add_includedirs("src")

if is_plat("windows") then
    set_runtimes(is_mode("debug") and "MDd" or "MD")
end

target("TSOM", function ()
    set_kind("binary")
    add_headerfiles("src/**.hpp", "src/**.inl")
    add_files("src/**.cpp")

    add_packages("nazaraengine", { components = { "graphics", "joltphysics3d", "widgets" } })
end)
