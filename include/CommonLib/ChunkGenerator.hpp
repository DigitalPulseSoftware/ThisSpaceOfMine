// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKGENERATOR_HPP
#define TSOM_COMMONLIB_CHUNKGENERATOR_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/Result.hpp>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace tsom
{
	class Chunk;

	class TSOM_COMMONLIB_API ChunkGenerator
	{
		public:
			ChunkGenerator(Nz::ApplicationBase& app);
			ChunkGenerator(const ChunkGenerator&) = delete;
			ChunkGenerator(ChunkGenerator&&) = delete;
			~ChunkGenerator();

			Nz::Result<std::vector<BlockIndex>, std::string> Generate(Chunk& chunk, const Nz::Vector3ui& chunkCount, const std::unordered_map<std::string, EntityProperty>& properties) const;

			inline const std::string& GetPlanetType() const;

			Nz::Result<void, std::string> Load(std::string_view scriptName);

			ChunkGenerator& operator=(const ChunkGenerator&) = delete;
			ChunkGenerator& operator=(ChunkGenerator&&) = delete;

		private:
			sol::protected_function m_generationFunction;
			std::string m_planetType;
			ScriptingContext m_scriptingContext;
			Nz::ApplicationBase& m_app;
	};
}

#include <CommonLib/ChunkGenerator.inl>

#endif // TSOM_COMMONLIB_CHUNKGENERATOR_HPP
