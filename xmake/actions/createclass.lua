local modules = {
	ClientLib = "library",
	CommonLib = "library",
	ServerLib = "library",
	Game = "standalone",
	Server = "standalone"
}

-- defined at the end of the file
local headerTemplate, inlineTemplate, sourceTemplate

task("create-class")

set_menu({
	-- Settings menu usage
	usage = "xmake create-class [options] name",
	description = "Helper for class creation",
	options =
	{
		-- Set k mode as key-only bool parameter
		{nil, "nocpp", "k", nil, "Set this if you don't want a .cpp to be created (header-only classes)" },
		{'m', "module", "v", nil, "Module where the class should be created" },
		{nil, "name", "v", nil, "Class name" }
	}
})

on_run(function ()
	import("core.base.option")

	local moduleName = option.get("module")
	if not moduleName then
		os.raise("missing module name")
	end

	local classPath = option.get("name")
	if not classPath then
		os.raise("missing class name")
	end
	
	local module = modules[moduleName]
	if not module then
		os.raise("unknown module " .. moduleName)
	end

	local className = path.basename(classPath)

	local includedir = (module == "library") and "include" or "src"

	local files = {
		{ TargetPath = path.join(includedir, moduleName, classPath) .. ".hpp", Template = headerTemplate },
		{ TargetPath = path.join(includedir, moduleName, classPath) .. ".inl", Template = inlineTemplate }
	}

	if not option.get("nocpp") then
		table.insert(files, { TargetPath = path.join("src", moduleName, classPath) .. ".cpp", Template = sourceTemplate })
	end

	local replacements = {
		CLASS_NAME = className,
		CLASS_PATH = classPath,
		COPYRIGHT = os.date("%Y") .. [[ Jérôme "SirLynix" Leclercq (lynix680@gmail.com)]],
		HEADER_GUARD = "TSOM_" .. moduleName:upper() .. "_" .. classPath:gsub("[/\\]", "_"):upper() .. "_HPP",
		MODULE_API = "TSOM_" .. moduleName:upper() .. "_API",
		MODULE_NAME = moduleName,
		BASE_HEADER = (module == "library") and moduleName .. "/Export.hpp" or "NazaraUtils/Prerequisites.hpp"
	}

	for _, file in pairs(files) do
		local content = file.Template:gsub("%%([%u_]+)%%", function (kw)
			local r = replacements[kw]
			if not r then
				os.raise("missing replacement for " .. kw)
			end

			return r
		end)

		io.writefile(file.TargetPath, content)
	end
end)

headerTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef %HEADER_GUARD%
#define %HEADER_GUARD%

#include <%BASE_HEADER%>

namespace tsom
{
	class %MODULE_API% %CLASS_NAME%
	{
		public:
			%CLASS_NAME%() = default;
			%CLASS_NAME%(const %CLASS_NAME%&) = delete;
			%CLASS_NAME%(%CLASS_NAME%&&) = delete;
			~%CLASS_NAME%() = default;

			%CLASS_NAME%& operator=(const %CLASS_NAME%&) = delete;
			%CLASS_NAME%& operator=(%CLASS_NAME%&&) = delete;

		private:
	};
}

#include <%MODULE_NAME%/%CLASS_PATH%.inl>

#endif // %HEADER_GUARD%
]]

inlineTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
}
]]

sourceTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <%MODULE_NAME%/%CLASS_PATH%.hpp>

namespace tsom
{
}
]]
