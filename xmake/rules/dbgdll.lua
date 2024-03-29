rule("dbgdll")
    after_install("windows", function (target)
        import("lib.detect.find_file")

        local msvc = target:toolchain("msvc")
        if msvc then
            local vcvars = msvc:config("vcvars")
            local subdirs = {
                path.join("Debuggers", target:arch())
            }
            local dbghelp = find_file("dbghelp.dll", vcvars.WindowsSdkDir, {suffixes = subdirs})
            if dbghelp then
                os.vcp(dbghelp, path.join(target:installdir(), "bin"))
            else
                print("dbghelp.dll not found!")
            end
        end
    end)
