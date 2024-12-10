// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <fmt/core.h>
#include <sol/state.hpp>

SOL_BASE_CLASSES(tsom::DeformedChunk, tsom::Chunk);
SOL_BASE_CLASSES(tsom::FlatChunk, tsom::Chunk);
SOL_DERIVED_CLASSES(tsom::Chunk, tsom::DeformedChunk, tsom::FlatChunk);

namespace tsom
{
	void ChunkScriptingLibrary::Register(sol::state& state)
	{
		RegisterBlockLibrary(state);
		RegisterChunk(state);
		RegisterChunkContainer(state);
	}

	void ChunkScriptingLibrary::RegisterBlockLibrary(sol::state& state)
	{
		state.new_usertype<BlockLibrary>("BlockLibrary",
			sol::no_constructor,
			"GetBlockIndex", LuaFunction([](sol::this_state L, const BlockLibrary& blockLibrary, std::string_view blockName) -> sol::object
			{
				BlockIndex blockIndex = blockLibrary.GetBlockIndex(blockName);
				if (blockIndex == InvalidBlockIndex)
					TriggerLuaArgError(L, 2, fmt::format("invalid block name {}", blockName));

				return sol::make_object(L, blockIndex);
			})
		);
	}

	void ChunkScriptingLibrary::RegisterChunk(sol::state& state)
	{
		state.new_usertype<Chunk>("Chunk",
			sol::no_constructor,
			"GetBlockLibrary", LuaFunction([](const Chunk& chunk)
			{
				return &chunk.GetBlockLibrary();
			}),
			"GetBlockContent", sol::overload(
				LuaFunction(Nz::Overload<unsigned int>(&Chunk::GetBlockContent)),
				LuaFunction(Nz::Overload<const Nz::Vector3ui&>(&Chunk::GetBlockContent))
			),
			"GetBlockCount", LuaFunction(&Chunk::GetBlockCount),
			"GetBlockLocalIndex", LuaFunction(&Chunk::GetBlockLocalIndex),
			"GetBlockLocalIndices", LuaFunction(&Chunk::GetBlockLocalIndices),
			"GetContainer", LuaFunction([](Chunk& chunk)
			{
				return &chunk.GetContainer();
			}),
			"GetContent", LuaFunction([](sol::this_state L, Chunk& chunk)
			{
				sol::state_view state(L);

				auto& blockLibrary = chunk.GetBlockLibrary();

				unsigned int blockCount = chunk.GetBlockCount();
				sol::table contentTable = state.create_table(blockCount);
				for (unsigned int i = 0; i < blockCount; ++i)
					contentTable[i + 1] = blockLibrary.GetBlockData(chunk.GetBlockContent(i)).name;

				return contentTable;
			}),
			"GetIndices", LuaFunction(&Chunk::GetIndices),
			"GetSize", LuaFunction(&Chunk::GetSize),
			"Reset", LuaFunction([](sol::this_state, Chunk& chunk, sol::table contentTable)
			{
				unsigned int blockCount = chunk.GetBlockCount();
				std::size_t contentSize = contentTable.size();
				if (contentSize != blockCount)
					fmt::print("Chunk:Reset called with a table containing {} entries, {} expected\n", contentSize, blockCount);

				auto& blockLibrary = chunk.GetBlockLibrary();

				chunk.Reset([&](BlockIndex* blockIndexPtr)
				{
					std::size_t maxEntries = std::min<std::size_t>(blockCount, contentSize);
					for (std::size_t i = 0; i < maxEntries; ++i)
					{
						BlockIndex blockIndex = contentTable[i + 1].get<BlockIndex>();
						if (!blockLibrary.IsValidBlock(blockIndex))
						{
							fmt::print("Chunk:Reset content table #{} contained invalid block index \"{}\"", i, blockIndex);
							blockIndex = EmptyBlockIndex;
						}

						*blockIndexPtr++ = blockIndex;
					}

					// If not enough blocks have been passed, fill the remaining with empty
					for (std::size_t i = maxEntries; i < blockCount; ++i)
						*blockIndexPtr++ = EmptyBlockIndex;
				});
			})
		);
	}

	void ChunkScriptingLibrary::RegisterChunkContainer(sol::state& state)
	{
		state.new_usertype<ChunkContainer>("ChunkContainer",
			sol::no_constructor,
			"GetBlockIndices", LuaFunction(&ChunkContainer::GetBlockIndices),
			"GetChunkCount", LuaFunction(&ChunkContainer::GetChunkCount),
			"GetChunkOffset", LuaFunction(&ChunkContainer::GetChunkOffset),
			"GetTileSize", LuaFunction(&ChunkContainer::GetTileSize)
		);
	}
}
