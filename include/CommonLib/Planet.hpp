// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLANET_HPP
#define TSOM_COMMONLIB_PLANET_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkGenerator.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/GravityController.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Core/ThreadLocalData.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <tsl/hopscotch_map.h>
#include <memory>
#include <mutex>

namespace Nz
{
	class ApplicationBase;
	class TaskScheduler;
}

namespace tsom
{
	class TSOM_COMMONLIB_API Planet : public ChunkContainer, public GravityController
	{
		public:
			Planet(Nz::ApplicationBase& app, float blockSize);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			Chunk& AddChunk(const BlockLibrary& blockLibrary, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);
			void AddChunks(const BlockLibrary& blockLibrary, const Nz::Vector3ui& chunkCount);

			void ClearChunks() override;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			void GenerateChunk(Chunk& chunk, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties);
			void GenerateChunks(Nz::TaskScheduler& taskScheduler, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties);
			void GeneratePlatform(const BlockLibrary& blockLibrary, Direction upDirection, const BlockIndices& platformCenter);

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const Chunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;

			void RemoveChunk(const ChunkIndices& indices) override;

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		protected:
			struct ChunkData
			{
				std::shared_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
				NazaraSlot(Chunk, OnClear, onClear);
				NazaraSlot(Chunk, OnLayerRegistered, onLayerRegistered);
				NazaraSlot(Chunk, OnLayerUnregistered, onLayerUnregistered);
				NazaraSlot(Chunk, OnReset, onReset);
			};

			std::mutex m_chunkLayerAddedSignalMutex;
			std::mutex m_chunkLayerRemovedSignalMutex;
			std::mutex m_chunkUpdatedSignalMutex;
			tsl::hopscotch_map<ChunkIndices, ChunkData> m_chunks;
			Nz::ThreadLocalData<ChunkGenerator> m_chunkGenerators;
			Nz::ApplicationBase& m_app;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_PLANET_HPP
