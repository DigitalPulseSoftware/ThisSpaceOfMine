includes("xmake/actions/*.lua")
includes("xmake/rules/*.lua")

-- Options
option("commonlib_static", { default = false, defines = "TSOM_COMMONLIB_STATIC"})
option("clientlib_static", { default = false, defines = "TSOM_CLIENTLIB_STATIC"})
option("serverlib_static", { default = false, defines = "TSOM_SERVERLIB_STATIC"})

-- Simple rule to make targets inherit their version from CommonLib version (which is extracted from git in on_config callback)
rule("inherit_version", function ()
	on_config(function (target)
		target:set("version", target:dep("CommonLib"):version())
	end)
end)

add_repositories("nazara-repo https://github.com/NazaraEngine/xmake-repo.git")
add_requires("nazaraengine >=2023.08.15", { configs = { debug = is_mode("debug"), symbols = true }})
add_requires("fmt", { configs = { header_only = false }})
add_requires("libcurl", { configs = { shared = true }, system = false })
add_requires(
	"concurrentqueue",
	"cppcodec",
	"libsodium",
	"lz4",
	"hopscotch-map",
	"nazarautils",
	"nlohmann_json",
	"perlinnoise",
	"semver",
	"sol2"
)

if is_plat("windows") then
	add_requires("stackwalker 5b0df7a4db8896f6b6dc45d36e383c52577e3c6b")
elseif is_plat("macosx") then
	add_requires("moltenvk", { configs = { shared = true }})
end

add_requireconfs("fmt", "stackwalker", { debug = is_mode("debug") })

-- Don't link with system-installed libs on CI
if os.getenv("CI") then
	add_requireconfs("*", { system = false })
end

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")
add_rules("dbgdll", "natvis")

--set_policy("package.requires_lock", true)

set_project("ThisSpaceOfMine")
set_version("0.0.0")

set_encodings("utf-8")
set_exceptions("cxx")
set_languages("cxx20")
set_rundir(".")
add_includedirs("include", "src")
set_targetdir("./bin/$(plat)_$(arch)_$(mode)")

if is_mode("debug") then
	add_defines("NAZARA_DEBUG")
	set_symbols("debug", "edit")
end

if is_plat("windows") then
	set_runtimes(is_mode("debug") and "MDd" or "MD")
end

add_rules("@nazarautils/compiler_setup")

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

	add_packages("nazaraengine", { components = { "physics3d", "network" }, public = true })
	add_packages("concurrentqueue", "cppcodec", "semver", "fmt", "hopscotch-map", "nlohmann_json", "sol2", { public = true })
	add_packages("libsodium", "lz4", "perlinnoise")

	if is_plat("windows") then
		add_packages("stackwalker")
	end

	on_config(function (target, opt)
		import("core.base.semver")
		import("detect.tools.find_git")

		-- extract latest release from git
		local ok = try
		{
			function ()
				local git = assert(find_git(), "git not found")
				local tag = os.iorunv(git, {"describe", "--abbrev=0", "--tags"}):trim()
				local version = semver.new(tag)
				target:set("version", version:shortstr())
			end,
			catch
			{
				function (err)
					print(string.format("failed to retrieve version from git: %s", err))
				end
			}
		}
	end)

	before_build(function (target, opt)
		import("core.base.semver")
		import("core.project.depend")
		import("detect.tools.find_git")
		import("utils.progress")

		local targetfile = "src/CommonLib/VersionData.hpp"

		local host = os.host()
		local subhost = os.subhost()

		local system
		if host ~= subhost then
			system = host .. "/" .. subhost
		else
			system = host
		end

		local branch = "unknown-branch"
		local commitHash = "unknown-commit"
		local commitDate = "unknown-date"
		local ok = try
		{
			function ()
				local git = assert(find_git(), "git not found")
				branch = os.iorunv(git, {"rev-parse", "--abbrev-ref", "HEAD"}):trim()
				commitHash = os.iorunv(git, {"describe", "--always", "--tags", "--long"}):trim()
				commitDate = os.iorunv(git, {"show", "--no-patch", "--format=%ci", commitHash}):trim()

				return true
			end,

			catch
			{
				function (err)
					print(string.format("Failed to retrieve git data: %s", err))
				end
			}
		}

		local targetversion = semver.new(target:version())

		local targetplat = target:plat()
		if targetplat == "macosx" then
			targetplat = "macos"
		end
		local buildconf = string.format("%s_%s", targetplat, target:arch())

		local dependfile = target:dependfile("versioninfo")
		depend.on_changed(function ()
			progress.show(opt.progress, "${color.build.target}updating version info (%s on %s@%s)", targetversion:shortstr(), commitHash, branch)
			io.writefile(targetfile, string.format([[
// this file was automatically generated
// no header guards

std::uint32_t GameMajorVersion = %s;
std::uint32_t GameMinorVersion = %s;
std::uint32_t GamePatchVersion = %s;
std::string_view BuildConfig = "%s";
std::string_view BuildSystem = "%s";
std::string_view BuildBranch = "%s";
std::string_view BuildCommit = "%s";
std::string_view BuildCommitDate = "%s";
]], targetversion:major(), targetversion:minor(), targetversion:patch(), buildconf, system, branch, commitHash, commitDate))
		end,
		{
			dependfile = dependfile, 
			files = targetfile,
			changed = target:is_rebuilt(),
			values = {targetversion:shortstr(), buildconf, system, branch, commitHash, commitDate}
		})
	end)

end)

target("ServerLib", function ()
	set_group("Common")
	set_basename("TSOMServer")
	add_headerfiles("include/(ServerLib/**.hpp)", "include/(ServerLib/**.inl)")
	add_headerfiles("src/ServerLib/**.hpp", "src/ServerLib/**.inl", { install = false })
	add_files("src/ServerLib/**.cpp")
	add_deps("CommonLib", { public = true })
	add_packages("libcurl", { links = {}, public = true })
	add_rules("inherit_version")

	after_load(function (target)
		target:set("kind", target:dep("serverlib_static") and "static" or "shared")
	end)

	add_defines("TSOM_SERVERLIB_BUILD")
	add_options("serverlib_static")
end)

target("ClientLib", function ()
	set_group("Common")
	set_basename("TSOMClient")
	add_headerfiles("include/(ClientLib/**.hpp)", "include/(ClientLib/**.inl)")
	add_headerfiles("src/ClientLib/**.hpp", "src/ClientLib/**.inl", { install = false })
	add_files("src/ClientLib/**.cpp")
	add_deps("CommonLib", { public = true })
	add_rules("inherit_version")

	after_load(function (target)
		target:set("kind", target:dep("clientlib_static") and "static" or "shared")
	end)

	add_defines("TSOM_CLIENTLIB_BUILD")
	add_options("clientlib_static")

	add_packages("nazaraengine", { components = { "audio", "graphics", "widgets" }, public = true })
end)

target("Main", function ()
	set_group("Common")
	set_basename("TSOMMain")
	set_kind("static")

	add_deps("CommonLib")
	add_rules("inherit_version")

	add_defines("TSOM_MAIN_BUILD")

	add_headerfiles("include/(Main/**.hpp)", "include/(Main/**.inl)")
	add_headerfiles("src/Main/**.hpp", "src/Main/**.inl")
	add_files("src/Main/**.cpp")
	add_packages("nazaraengine", { components = { "core" }, public = true })
end)

target("TSOMGame", function ()
	set_group("Executable")
	set_basename("ThisSpaceOfMine")
	add_deps("ClientLib", "Main")
	add_rules("inherit_version")

	add_defines("TSOM_GAME_BUILD")

	add_headerfiles("src/Game/**.hpp", "src/Game/**.inl")
	add_files("src/Game/**.cpp")
	add_installfiles("gameconfig.lua.default", { prefixdir = "bin" })

	if is_plat("windows", "mingw") then
		add_files("src/Game/resources.rc")
	end

	add_rpathdirs("@executable_path")

	add_packages("nazaraengine", { components = { "widgets" }, public = true })
	add_packages("libcurl", { links = {} })
	if is_plat("macosx") then
		add_packages("moltenvk", { links = {} })
	end

	after_install(function (target)
		local curl = target:pkg("libcurl")
		if not curl then
			return
		end

		local bin = path.join(curl:installdir(), "bin")
		os.vcp(path.join(bin, "*.dll"), target:installdir("bin"))
		os.vcp(path.join(bin, "*.so"), target:installdir("bin"))
		os.vcp(path.join(bin, "*.dynlib"), target:installdir("bin"))
	end)
end)

target("TSOMServer", function ()
	set_group("Executable")
	set_basename("ThisServerOfMine")
	add_deps("ServerLib", "Main")
	add_rules("inherit_version")

	add_defines("TSOM_SERVER_BUILD")

	add_headerfiles("src/Server/**.hpp", "src/Server/**.inl")
	add_files("src/Server/**.cpp")
	add_installfiles("serverconfig.lua.default", { prefixdir = "bin" })

	add_rpathdirs("@executable_path")
end)

includes("tests/xmake.lua")
