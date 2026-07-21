// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkGenerator.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingProperties.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ChunkGenerator::ChunkGenerator(Nz::ApplicationBase& app) :
	m_scriptingContext(app),
	m_app(app)
	{
		m_scriptingContext.RegisterLibrary<BaseScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<ChunkScriptingLibrary>();

		m_scriptingContext.LoadDirectory("scripts/libraries");
	}

	ChunkGenerator::~ChunkGenerator()
	{
		m_generationFunction = sol::lua_nil;
	}

	Nz::Result<std::vector<BlockIndex>, std::string> ChunkGenerator::Generate(Chunk& chunk, const Nz::Vector3ui& chunkCount, const std::unordered_map<std::string, EntityProperty>& properties) const
	{
		sol::state_view state = m_scriptingContext.GetState();

		sol::table propertyTable = state.create_table(0, int(properties.size()));
		for (const auto& [propertyName, propertyValue] : properties)
			propertyTable[propertyName] = TranslatePropertyToLua(state, propertyValue);

		auto result = m_generationFunction(chunk, chunkCount, propertyTable);
		if (!result.valid())
		{
			sol::error err = result;
			return Nz::Err(err.what());
		}

		ChunkIndices chunkIndices = chunk.GetIndices();

		sol::object resultObject = result;
		if (!resultObject.is<sol::table>())
			return Nz::Err(fmt::format("result is not a table (got {})", sol::type_name(state, resultObject.get_type())));

		std::size_t blockCount = chunk.GetBlockCount();

		sol::table blockTable = result;

		std::size_t contentSize = blockTable.size();
		if (contentSize != blockCount)
			spdlog::error("Chunk generator returned a table containing {} entries, {} expected", contentSize, blockCount);

		auto& blockLibrary = chunk.GetBlockLibrary();

		std::vector<BlockIndex> blocks(blockCount, EmptyBlockIndex);
		std::size_t maxEntries = std::min<std::size_t>(blockCount, contentSize);
		for (std::size_t i = 0; i < maxEntries; ++i)
		{
			BlockIndex blockIndex = blockTable[i + 1].get<BlockIndex>();
			if (!blockLibrary.IsValidBlock(blockIndex))
			{
				spdlog::error("Chunk generator table #{} contained invalid block index \"{}\"", i, blockIndex);
				blockIndex = EmptyBlockIndex;
			}

			blocks[i] = blockIndex;
		}

		return Nz::Ok(std::move(blocks));
	}

	Nz::Result<void, std::string> ChunkGenerator::Load(std::string_view scriptName)
	{
		NAZARA_TRY_VALUE(sol::table generatorTable, m_scriptingContext.LoadFile(fmt::format("scripts/planets/{}.lua", scriptName)));

		m_planetType = generatorTable["PlanetType"];
		m_generationFunction = generatorTable["Generator"];

		return Nz::Ok();
	}
}
