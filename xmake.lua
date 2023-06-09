
-- Should the CommonLib be compiled statically? (takes more space)
option("commonlib_static", { default = false, defines = "TSOM_COMMONLIB_STATIC"})
option("clientlib_static", { default = false, defines = "TSOM_CLIENTLIB_STATIC"})

add_repositories("nazara-repo https://github.com/NazaraEngine/xmake-repo.git")
add_requires("nazaraengine", { configs = { debug = is_mode("debug"), with_symbols = true }})
add_requires("fmt", { configs = { header_only = false }})
add_requires("concurrentqueue", "hopscotch-map", "nlohmann_json")

if is_plat("windows") then
	add_requires("stackwalker 5b0df7a4db8896f6b6dc45d36e383c52577e3c6b")
end

add_requireconfs("fmt", "stackwalker", { debug = is_mode("debug", "asan") })

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

--set_policy("package.requires_lock", true)

set_project("ThisSpaceOfMine")
set_version("0.0.1")

set_languages("cxx20")
set_rundir(".")
add_includedirs("include", "src")

if is_plat("windows") then
	set_runtimes(is_mode("debug") and "MDd" or "MD")
	add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
end

target("CommonLib", function ()
	set_group("Common")
	set_basename("TSOMCommon")
	add_headerfiles("include/(CommonLib/**.hpp)", "include/(CommonLib/**.inl)")
	add_headerfiles("src/CommonLib/**.hpp", "src/CommonLib/**.inl", { install = false })
	add_files("src/CommonLib/**.cpp")

	after_load(function (target)
		target:set("kind", target:dep("commonlib_static") and "static" or "shared")
	end)

	add_defines("TSOM_COMMONLIB_BUILD")
	add_options("commonlib_static")

	add_packages("nazaraengine", { components = { "joltphysics3d", "network", "utility" }, public = true })
	add_packages("concurrentqueue", "fmt", "hopscotch-map", "nlohmann_json", { public = true })

	if is_plat("windows") then
		add_packages("stackwalker")
	end

	before_build(function (target)
		local host = os.host()
		local subhost = os.subhost()

		local system
		if (host ~= subhost) then
			system = host .. "/" .. subhost
		else
			system = host
		end

		local branch = "unknown-branch"
		local commitHash = "unknown-commit"
		try
		{
			function ()
				import("detect.tools.find_git")
				local git = find_git()
				if (git) then
					branch = os.iorunv(git, {"rev-parse", "--abbrev-ref", "HEAD"}):trim()
					commitHash = os.iorunv(git, {"describe", "--tags", "--long"}):trim()
				else
					error("git not found")
				end
			end,

			catch
			{
				function (err)
					print(string.format("Failed to retrieve git data: %s", err))
				end
			}
		}

		import("core.project.depend")
		import("core.project.project")
		local tmpfile = path.join(os.projectdir(), "project.autoversion")
		local dependfile = tmpfile .. ".d"
		depend.on_changed(function ()
			print("regenerating version data info...")
			io.writefile("src/CommonLib/VersionData.hpp", string.format([[
const char* BuildSystem = "%s";
const char* BuildBranch = "%s";
const char* BuildCommit = "%s";
const char* BuildDate = "%s";
]], system, branch, commitHash, os.date("%Y-%m-%d %H:%M:%S")))
		end, 
		{
			dependfile = dependfile, 
			files = project.allfiles(), 
			values = {system, branch, commitHash}
		})
	end)

end)

target("ClientLib", function ()
	set_group("Common")
	set_basename("TSOMClient")
	add_headerfiles("include/(ClientLib/**.hpp)", "include/(ClientLib/**.inl)")
	add_headerfiles("src/ClientLib/**.hpp", "src/ClientLib/**.inl", { install = false })
	add_files("src/ClientLib/**.cpp")
	add_deps("CommonLib", { public = true })

	after_load(function (target)
		target:set("kind", target:dep("clientlib_static") and "static" or "shared")
	end)

	add_defines("TSOM_CLIENTLIB_BUILD")
	add_options("clientlib_static")

	add_packages("nazaraengine", { components = { "audio", "graphics" }, public = true })
end)

target("Main")
	set_group("Common")
	set_basename("TSOMMain")
	set_kind("static")

	add_defines("TSOM_MAIN_BUILD")

	add_deps("CommonLib")
	add_headerfiles("include/(Main/**.hpp)", "include/(Main/**.inl)")
	add_headerfiles("src/Main/**.hpp", "src/Main/**.inl")
	add_files("src/Main/**.cpp")
	add_packages("nazaraengine", { components = { "core" }, public = true })

target("TSOMGame", function ()
	set_group("Executable")
	set_basename("ThisSpaceOfMine")
	add_deps("ClientLib", "Main")

	add_defines("TSOM_GAME_BUILD")

	add_headerfiles("src/Game/**.hpp", "src/Game/**.inl")
	add_files("src/Game/**.cpp")

	add_packages("nazaraengine", { components = { "widgets" }, public = true })
end)

target("TSOMServer", function ()
	set_group("Executable")
	set_basename("ThisServerOfMine")
	add_deps("CommonLib", "Main")

	add_defines("TSOM_SERVER_BUILD")

	add_headerfiles("src/Server/**.hpp", "src/Server/**.inl")
	add_files("src/Server/**.cpp")

	add_packages("nazaraengine", { components = { "core" } })
end)
