local modules = {
	ClientLib = "library",
	CommonLib = "library",
	ServerLib = "library",
	Game = "standalone",
	Server = "standalone"
}

task("check-files")

set_menu({
	-- Settings menu usage
	usage = "xmake check-files [options]",
	description = "Check every file for consistency (can fix some errors)",
	options =
	{
		-- Set k mode as key-only bool parameter
		{'f', "fix", "k", nil, "Attempt to automatically fix files." }
	}
})

local function CompareLines(referenceLines, lines, firstLine, lineCount)
	firstLine = firstLine or 1
	lineCount = lineCount or (#lines - firstLine + 1)
	if lineCount ~= #referenceLines then
		return false
	end

	for i = 1, lineCount do
		if lines[firstLine + i - 1] ~= referenceLines[i] then
			return false
		end
	end

	return true
end

local function ComputeHeaderFile(filePath)
	local headerPath = filePath:gsub("[\\/]", "/")
	if headerPath:startswith("src/") then
		headerPath = headerPath:sub(5)
	end
	headerPath = headerPath:sub(1, headerPath:find("%..+$") - 1) .. ".hpp"

	return headerPath
end

local systemHeaders = {
	["fcntl.h"] = true,
	["mstcpip"] = true,
	["netdb.h"] = true,
	["poll.h"] = true,
	["process.h"] = true,
	["pthread.h"] = true,
	["signal.h"] = true,
	["spawn.h"] = true,
	["unistd.h"] = true,
	["windows.h"] = true,
	["winsock2.h"] = true,
	["ws2tcpip.h"] = true,
}

local function IsProjectHeader(header)
	local folder = header:match("^(.-)/")
	return modules[folder] ~= nil
end

local function IsSystemHeader(header)
	if systemHeaders[header:lower()] then
		return true
	end

	if header:match("netinet/.*") then
		return true
	end

	if header:match("sys/.*") then
		return true
	end

	return false
end

on_run(function ()
	import("core.base.option")

	local fileLines = {}
	local updatedFiles = {}
	local function GetFile(filePath)
		filePath = path.translate(filePath)

		local lines = fileLines[filePath]
		if not lines then
			lines = table.to_array(io.lines(filePath))
			if not lines then
				os.raise("failed to open " .. filePath)
			end

			fileLines[filePath] = lines
		end

		return lines
	end

	local function UpdateFile(filePath, lines)
		filePath = path.translate(filePath)

		if lines then
			fileLines[filePath] = lines
		end

		updatedFiles[filePath] = true
	end

	local checks = {}

	-- Remove empty lines at the beginning of files
	table.insert(checks, {
		Name = "empty lines",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp"),
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local fixes = {}

			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				for i = 1, #lines do
					if not lines[i]:match("^%s*$") then
						if i ~= 1 then
							print(filePath .. " starts with empty lines")

							table.insert(fixes, {
								File = filePath,
								Func = function (lines)
									for j = 1, i - 1 do
										table.remove(lines, 1)
									end

									UpdateFile(filePath, lines)
								end
							})
						end
						
						break
					end
				end
			end

			return fixes
		end
	})

	-- Check header guards and #pragma once
	table.insert(checks, {
		Name = "header guards",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.hpp")
			)

			local fixes = {}
			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local pragmaLine
				local ifndefLine
				local defineLine
				local endifLine
				local macroName

				local pathMacro = filePath:gsub("[/\\]", "_")
				do
					pathMacro = pathMacro:sub(pathMacro:lastof(moduleName .. "_", true) + #moduleName + 1)
					local i = pathMacro:lastof(".", true)
					if i then
						pathMacro = pathMacro:sub(1, i - 1)
					end
				end

				local pathHeaderGuard = (moduleName ~= pathMacro) and "TSOM_" .. moduleName:upper() .. "_" .. pathMacro:upper() .. "_HPP" or "TSOM_" .. moduleName:upper() .. "_HPP"

				local canFix = true
				local ignored = false

				-- Fetch pragma once, ifdef and define lines
				for i = 1, #lines do
					if lines[i] == "// no header guards" then
						canFix = false
						ignored = true
						break
					end

					if lines[i] == "#pragma once" then
						if pragmaLine then
							print(filePath .. ": multiple #pragma once found")
							canFix = false
							break
						end

						pragmaLine = i
					elseif not ifndefLine and lines[i]:startswith("#ifndef") then
						ifndefLine = i

						macroName = lines[i]:match("^#ifndef%s+(.+)$")
						if not macroName then
							print(filePath .. ": failed to identify header guard macro (ifndef)")
							canFix = false
							break
						end
					elseif ifndefLine and not defineLine and lines[i]:startswith("#define") then
						defineLine = i

						local defineMacroName = lines[i]:match("^#define%s+(.+)$")
						if not defineMacroName then
							print(filePath .. ": failed to identify header guard macro (define)")
							canFix = false
							break
						end

						if defineMacroName ~= macroName then
							print(filePath .. ": failed to identify header guard macro (define macro doesn't match ifdef)")
							canFix = false
							break
						end
					end

					if ifndefLine and defineLine then
						break
					end
				end

				if not ignored then
					if not ifndefLine or not defineLine or not macroName then
						print(filePath .. ": failed to identify header guard macro")
						canFix = false
					end

					-- Fetch endif line
					if canFix then
						local shouldFixEndif = false

						for i = #lines, 1, -1 do
							if lines[i]:startswith("#endif") then
								local macro = lines[i]:match("#endif // (.+)")
								if macro ~= macroName then
									shouldFixEndif = true
								end

								endifLine = i
								break
							end
						end

						if not endifLine then
							print(filePath .. ": failed to identify header guard macro (endif)")
							canFix = false
						end

						if canFix then
							if macroName ~= pathHeaderGuard then
								print(filePath .. ": header guard mismatch (got " .. macroName .. ", expected " .. pathHeaderGuard .. ")")

								shouldFixEndif = false

								table.insert(fixes, {
									File = filePath,
									Func = function (lines)
										lines[ifndefLine] = "#ifndef " .. pathHeaderGuard
										lines[defineLine] = "#define " .. pathHeaderGuard
										lines[endifLine] = "#endif // " .. pathHeaderGuard

										return lines
									end
								})
							end

							if shouldFixEndif then
								print(filePath .. ": #endif was missing comment")

								table.insert(fixes, {
									File = filePath,
									Func = function (lines)
										lines[endifLine] = "#endif // " .. pathHeaderGuard

										return lines
									end
								})
							end

							if not pragmaLine then
								print(filePath .. ": no #pragma once found")
								table.insert(fixes, {
									File = filePath,
									Func = function (lines)
										table.insert(lines, ifndefLine - 1, "#pragma once")
										table.insert(lines, ifndefLine - 1, "")

										return lines
									end
								})
							elseif pragmaLine > ifndefLine then
								print(filePath .. ": #pragma once is after header guard (should be before)")
							end
						end
					end
				end
			end

			return fixes
		end
	})

	-- Every source file should include its header first, except .inl files
	table.insert(checks, {
		Name = "inclusion",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local fixes = {}
			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local headerPath = ComputeHeaderFile(filePath)
				if os.isfile("include/" .. headerPath) or os.isfile("src/" .. headerPath) then
					local inclusions = {}

					-- Retrieve all inclusions
					for i = 1, #lines do
						if lines[i] == "// no include fix" then
							-- ignore file
							inclusions = {}
							break
						end

						local includeMode, includePath = lines[i]:match("^#include%s*([<\"])(.+)[>\"]")
						if includeMode then
							table.insert(inclusions, {
								line = i,
								mode = includeMode,
								path = includePath
							})
						end
					end

					if #inclusions > 0 then
						local increment = 0

						-- Add corresponding header
						local headerInclude
						for i = 1, #inclusions do
							if inclusions[i].path == headerPath then
								headerInclude = i
								break
							end
						end

						-- Add header inclusion if it's missing
						local isInl = path.extension(filePath) == ".inl"
						if not headerInclude and not isInl then
							print(filePath .. " is missing corresponding header inclusion")

							table.insert(fixes, {
								File = filePath,
								Func = function (lines)
									local firstHeaderLine = inclusions[1].line
									table.insert(lines, firstHeaderLine, "#include <" .. headerPath .. ">")

									increment = increment + 1

									return lines
								end
							})
						elseif headerInclude and isInl then
							print(filePath .. " has a header inclusion which breaks clangd (.inl should no longer includes their .hpp)")

							table.insert(fixes, {
								File = filePath,
								Func = function (lines)
									table.remove(lines, inclusions[headerInclude].line)
									increment = increment - 1
									return lines
								end
							})
						end
					end
				end
			end

			return fixes
		end
	})

	-- Reorder includes and remove duplicates
	table.insert(checks, {
		Name = "inclusion order",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp"),
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local fixes = {}
			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local headerPath = ComputeHeaderFile(filePath)
				
				local inclusions = {}

				-- Retrieve all inclusions from the first inclusion block
				for i = 1, #lines do
					if lines[i] == "// no include reordering" then
						-- ignore file
						inclusions = {}
						break
					end

					local includeMode, includePath = lines[i]:match("^#include%s*([<\"])(.+)[>\"]")
					if includeMode then
						table.insert(inclusions, {
							line = i,
							mode = includeMode,
							path = includePath
						})
					elseif #inclusions > 0 then
						-- Stop when outside the inclusion block
						break
					end
				end

				local includeList = {}
				local shouldReorder = false
				for i = 1, #inclusions do
					local order
					if inclusions[i].path == headerPath then
						order = 0 -- own include comes first
					elseif inclusions[i].path:endswith("Export.hpp") then
						order = 1 -- export include
					elseif IsProjectHeader(inclusions[i].path) then
						order = 2 -- own module
					elseif inclusions[i].path:match("^NazaraUtils/") then
						order = 3.1 -- NazaraUtils
					elseif inclusions[i].path:match("^Nazara/") then
						order = 3 -- Nazara
					elseif IsSystemHeader(inclusions[i].path) then
						order = 6 -- system includes
					elseif inclusions[i].path:match(".+%.hp?p?") then
						order = 4 -- thirdparty includes
					else
						order = 5 -- standard includes
					end

					table.insert(includeList, {
						order = order,
						path = inclusions[i].path,
						content = lines[inclusions[i].line]
					})
				end

				local function compareFunc(a, b)
					if a.order == b.order then
						local folderA = a.path:match("^(.-)/")
						local folderB = b.path:match("^(.-)/")
						if folderA and folderB then
							if folderA ~= folderB then
								return folderA < folderB
							end
						end

						local _, folderCountA = a.path:gsub("/", "")
						local _, folderCountB = b.path:gsub("/", "")
						if folderCountA == folderCountB then
							return a.path < b.path
						else
							return folderCountA < folderCountB
						end
					else
						return a.order < b.order
					end
				end

				local isOrdered = true
				for i = 2, #includeList do
					if includeList[i - 1].path == includeList[i].path then
						-- duplicate found
						print(filePath .. ": include list has duplicates")
						isOrdered = false
						break
					end

					if not compareFunc(includeList[i - 1], includeList[i]) then
						print(filePath .. ": include list is not ordered")
						isOrdered = false
						break
					end
				end

				if not isOrdered then
					table.sort(includeList, compareFunc)

					table.insert(fixes, {
						File = filePath,
						Func = function (lines)
							-- Reorder includes
							local newInclusions = {}
							for i = 1, #inclusions do
								lines[inclusions[i].line] = includeList[i].content
								table.insert(newInclusions, {
									content = includeList[i].content,
									path = includeList[i].path,
									line = inclusions[i].line
								})
							end

							-- Remove duplicates
							table.sort(newInclusions, function (a, b) return a.line > b.line end)

							for i = 2, #newInclusions do
								local a = newInclusions[i - 1]
								local b = newInclusions[i]

								if a.path == b.path then
									if #a.content > #b.content then -- keep longest line (for comments)
										table.remove(lines, b.line)
									else
										table.remove(lines, a.line)
									end
								end
							end

							return lines
						end
					})
				end
			end

			return fixes
		end
	})

	-- Check copyright date and format
	table.insert(checks, {
		Name = "copyright",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp|Config.hpp"),
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local currentYear = os.date("%Y")
			local projectAuthor = "Jérôme \"SirLynix\" Leclercq (lynix680@gmail.com)"
			local prevAuthor = "Jérôme \"Lynix\" Leclercq"
			local fixes = {}

			-- Headers
			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local hasCopyright
				local shouldFix = false

				if lines[1]:lower():match("^// this file was automatically generated") then
					goto skip
				end

				local year, authors = lines[1]:match("^// Copyright %(C%) (Y?E?A?R?%d*) (.+)$")
				hasCopyright = year ~= nil

				if not authors or authors == "AUTHORS" then
					authors = projectAuthor
				else
					local fixedAuthors = authors:gsub(prevAuthor, projectAuthor)
					if fixedAuthors ~= authors then
						authors = fixedAuthors
						shouldFix = true
					end
				end

				if hasCopyright then
					if year ~= currentYear then
						print(filePath .. ": copyright year error")
						shouldFix = true
					end

					if lines[2] ~= "// This file is part of the \"This Space Of Mine\" project" then
						print(filePath .. ": copyright project error")
						shouldFix = true
					end

					if lines[3] ~= "// For conditions of distribution and use, see copyright notice in LICENSE" then
						print(filePath .. ": copyright file reference error")
						shouldFix = true
					end
				else
					print(filePath .. ": copyright not found")
					shouldFix = true
				end

				if shouldFix then
					table.insert(fixes, {
						File = filePath,
						Func = function(lines)
							local copyrightLines = {
								"// Copyright (C) " .. currentYear .. " " .. (authors or projectAuthor),
								"// This file is part of the \"This Space Of Mine\" project",
								"// For conditions of distribution and use, see copyright notice in LICENSE"
							}

							if hasCopyright then
								for i, line in ipairs(copyrightLines) do
									lines[i] = line
								end
							else
								for i, line in ipairs(copyrightLines) do
									table.insert(lines, i, line)
								end
								table.insert(lines, #copyrightLines + 1, "")
							end

							return lines
						end
					})
				end

				::skip::
			end

			return fixes
		end
	})

	-- No space should lies before a linefeed
	table.insert(checks, {
		Name = "end of line spaces",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp"),
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local fixes = {}

			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local fileFixes = {}
				for i = 1, #lines do
					local content = lines[i]:match("^(%s*[^%s]*)%s+$")
					if content then
						table.insert(fileFixes, { line = i, newContent = content })
					end
				end

				if #fileFixes > 0 then
					print(filePath .. " has line ending with spaces")
					table.insert(fixes, {
						File = filePath,
						Func = function (lines)
							for _, fix in ipairs(fileFixes) do
								lines[fix.line] = fix.newContent
							end

							UpdateFile(filePath, lines)
						end
					})
				end
			end

			return fixes
		end
	})

	-- No tab character should exist after indentation
	table.insert(checks, {
		Name = "tab outside of indent",
		Check = function (moduleName)
			local files = table.join(
				os.files("include/" .. moduleName .. "/**.hpp"),
				os.files("include/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.hpp"),
				os.files("src/" .. moduleName .. "/**.inl"),
				os.files("src/" .. moduleName .. "/**.cpp")
			)

			local fixes = {}

			for _, filePath in pairs(files) do
				local lines = GetFile(filePath)

				local fileFixes = {}
				for i = 1, #lines do
					local start = lines[i]:match("\t*[^\t+]()\t")
					if start then
						table.insert(fileFixes, { line = i, start = start })
					end
				end

				if #fileFixes > 0 then
					print(filePath .. " has tab character outside of indentation")
					table.insert(fixes, {
						File = filePath,
						Func = function (lines)
							for _, fix in ipairs(fileFixes) do
								-- compute indent taking tabs into account
								local function ComputeIndent(str, i)
									local indent = 0
									for i = 1, i do
										if str:sub(i, i) == "\t" then
											-- round up to tabSize (4)
											indent = ((indent + 4) // 4) * 4
										else
											indent = indent + 1
										end
									end

									return indent
								end

								lines[fix.line] = lines[fix.line]:gsub("()\t", function (pos)
									if pos < fix.start then
										return
									end
									local indent = ComputeIndent(lines[fix.line], pos - 1)
									local indent2 = ComputeIndent(lines[fix.line], pos)
									return string.rep(" ", indent2 - indent)
								end)
							end

							UpdateFile(filePath, lines)
						end
					})
				end
			end

			return fixes
		end
	})

	local shouldFix = option.get("fix") or false

	for _, check in pairs(checks) do
		print("Running " .. check.Name .. " check...")

		local fixes = {}
		for moduleName, type in pairs(modules) do
			table.join2(fixes, check.Check(moduleName))
		end

		if shouldFix then
			for _, fix in pairs(fixes) do
				print("Fixing " .. fix.File)
				UpdateFile(fix.File, fix.Func(assert(fileLines[fix.File])))
			end
		end
	end

	for filePath, _ in pairs(updatedFiles) do
		local lines = assert(fileLines[filePath])
		if lines[#lines] ~= "" then
			table.insert(lines, "")
		end

		print("Saving changes to " .. filePath)
		io.writefile(filePath, table.concat(lines, "\n"))
	end
end)
