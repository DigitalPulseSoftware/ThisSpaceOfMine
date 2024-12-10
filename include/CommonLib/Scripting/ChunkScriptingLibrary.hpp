// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_CHUNKSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_CHUNKSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ChunkScriptingLibrary : public ScriptingLibrary
	{
		public:
			ChunkScriptingLibrary() = default;
			ChunkScriptingLibrary(const ChunkScriptingLibrary&) = delete;
			ChunkScriptingLibrary(ChunkScriptingLibrary&&) = delete;
			~ChunkScriptingLibrary() = default;

			void Register(sol::state& state) override;

			ChunkScriptingLibrary& operator=(const ChunkScriptingLibrary&) = delete;
			ChunkScriptingLibrary& operator=(ChunkScriptingLibrary&&) = delete;

		private:
			void RegisterBlockLibrary(sol::state& state);
			void RegisterChunk(sol::state& state);
			void RegisterChunkContainer(sol::state& state);
	};
}

#include <CommonLib/Scripting/ChunkScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_CHUNKSCRIPTINGLIBRARY_HPP
