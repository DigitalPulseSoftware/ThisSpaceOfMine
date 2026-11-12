// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ASTEROID_HPP
#define TSOM_COMMONLIB_ASTEROID_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkGenerator.hpp>
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
	class TSOM_COMMONLIB_API Asteroid : public ChunkContainer
	{
		public:
			Asteroid(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float blockSize, float radius, Nz::UInt32 seed);
			Asteroid(const Asteroid&) = delete;
			Asteroid(Asteroid&&) = delete;
			~Asteroid() = default;

			Chunk& AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr) override;
			void AddChunks(const Nz::Vector3ui& chunkCount);

			void ClearChunks() override;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			void GenerateChunks();

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const Chunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;

			void RemoveChunk(const ChunkIndices& indices) override;

			Asteroid& operator=(const Asteroid&) = delete;
			Asteroid& operator=(Asteroid&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		private:
			void GenerateChunk(Chunk& chunk, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties);

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
			Nz::UInt32 m_seed;
			Nz::ApplicationBase& m_app;
			float m_radius;
	};
}

#include <CommonLib/Asteroid.inl>

#endif // TSOM_COMMONLIB_ASTEROID_HPP
