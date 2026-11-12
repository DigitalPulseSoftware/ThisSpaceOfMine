// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Asteroid.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	Asteroid::Asteroid(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float blockSize, float radius, Nz::UInt32 seed) :
	ChunkContainer(blockLibrary, blockSize),
	m_seed(seed),
	m_app(app),
	m_radius(radius)
	{
	}

	Chunk& Asteroid::AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_unique<SurfaceNetsChunk>(*this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		chunkData.onLayerRegistered.Connect(chunkData.chunk->OnLayerRegistered, [this](Chunk* chunk, std::size_t layerIndex)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkLayerAddedSignalMutex);
			OnChunkLayerAdded(this, chunk, layerIndex);
		});

		chunkData.onLayerUnregistered.Connect(chunkData.chunk->OnLayerUnregistered, [this](Chunk* chunk, std::size_t layerIndex)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkLayerRemovedSignalMutex);
			OnChunkLayerRemove(this, chunk, layerIndex);
		});

		chunkData.onClear.Connect(chunkData.chunk->OnClear, [this](Chunk* chunk, Nz::UInt32 previousActiveLayerMask)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, previousActiveLayerMask);
		});

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this](Chunk* chunk)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, chunk->GetActiveLayerMask());
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex /*oldBlock*/, BlockIndex newBlock, std::size_t oldLayerIndex, std::size_t newLayerIndex)
		{
			NeighborChunkMask neighborMask;

			// Find every neighbor chunk required, based on block position
			std::array<Nz::Int32, 2> xs{ 0, 0 };
			std::array<Nz::Int32, 2> ys{ 0, 0 };
			std::array<Nz::Int32, 2> zs{ 0, 0 };

			std::size_t nx = 1, ny = 1, nz = 1;

			const Nz::Vector3ui& size = chunk->GetSize();

			if (indices.x == 0)
				xs[nx++] = -1;
			else if (indices.x == size.x - 1)
				xs[nx++] = 1;

			if (indices.y == 0)
				zs[nz++] = -1;
			else if (indices.y == size.y - 1)
				zs[nz++] = 1;

			if (indices.z == 0)
				ys[ny++] = -1;
			else if (indices.z == size.z - 1)
				ys[ny++] = 1;

			for (std::size_t ix = 0; ix < nx; ++ix)
			{
				for (std::size_t iy = 0; iy < ny; ++iy)
				{
					for (std::size_t iz = 0; iz < nz; ++iz)
					{
						Nz::Int32 dx = xs[ix];
						Nz::Int32 dy = ys[iy];
						Nz::Int32 dz = zs[iz];

						if (dx == 0 && dy == 0 && dz == 0)
							continue;

						ChunkIndices dir(dx, dy, dz);
						neighborMask |= ToNeighborChunk({ dx, dy, dz });
					}
				}
			}

			Nz::UInt32 layerMask = 0;
			layerMask |= 1u << oldLayerIndex;
			if (newBlock != EmptyBlockIndex)
				layerMask |= 1u << newLayerIndex;

			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, neighborMask, layerMask);
		});

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		if (initCallback)
			it->second.chunk->Reset(initCallback);

		Chunk* chunk = it->second.chunk.get();
		OnChunkAdded(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerAdded(this, chunk, layerIndex);

		return *it->second.chunk;
	}

	void Asteroid::AddChunks(const Nz::Vector3ui& chunkCount)
	{
		for (int chunkZ = 0; chunkZ < chunkCount.z; ++chunkZ)
		{
			for (int chunkY = 0; chunkY < chunkCount.y; ++chunkY)
			{
				for (int chunkX = 0; chunkX < chunkCount.x; ++chunkX)
					AddChunk({ chunkX - int(chunkCount.x / 2), chunkY - int(chunkCount.y / 2), chunkZ - int(chunkCount.z / 2) });
			}
		}
	}

	void Asteroid::ClearChunks()
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
		{
			Chunk* chunk = chunkData.chunk.get();
			OnChunkRemove(this, chunk);
			for (std::size_t layerIndex : chunk->GetActiveLayers())
				OnChunkLayerRemove(this, chunk, layerIndex);
		}

		m_chunks.clear();
	}

	void Asteroid::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Asteroid::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Asteroid::GenerateChunks()
	{
		constexpr std::string_view scriptName = "asteroid";

		std::unordered_map<std::string, EntityProperty> generatorProperties;
		generatorProperties["Radius"] = EntityPropertySingleValue<EntityPropertyType::Float>(m_radius);
		generatorProperties["Seed"] = EntityPropertySingleValue<EntityPropertyType::Integer>(m_seed);

		Nz::Vector3ui chunkCount((static_cast<unsigned int>((m_radius * 3.0 + 1.0) / m_tileSize) + ChunkSize - 1) / ChunkSize);

		ClearChunks();
		AddChunks(chunkCount);

		auto& taskScheduler = m_app.GetComponent<Nz::TaskSchedulerAppComponent>();

		m_chunkGenerators.Clear();
		ForEachChunk([&](const ChunkIndices& /*chunkIndices*/, Chunk& chunk)
		{
			if (chunk.HasContent())
				return;

			taskScheduler.AddTask([=, this, asteroidPtr = shared_from_this(), chunkPtr = chunk.shared_from_this(), props = generatorProperties]
			{
				GenerateChunk(*chunkPtr, chunkCount, scriptName, props);
			});
		});
	}

	void Asteroid::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		Chunk* chunk = it->second.chunk.get();
		OnChunkRemove(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerRemove(this, chunk, layerIndex);

		m_chunks.erase(it);
	}

	void Asteroid::GenerateChunk(Chunk& chunk, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties)
	{
		ChunkIndices chunkIndices = chunk.GetIndices();

		bool created;
		ChunkGenerator& chunkGenerator = m_chunkGenerators.GetOrCreate(created, m_app);
		if (created)
		{
			if (!chunkGenerator.Load(scriptName))
				return;
		}

		auto result = chunkGenerator.Generate(chunk, chunkCount, properties);
		if (!result)
		{
			spdlog::error("Chunk generator failed for {};{};{}: {}", chunkIndices.x, chunkIndices.y, chunkIndices.z, result.GetError());
			return;
		}

		std::vector<BlockIndex> blocks = std::move(result).GetValue();

		ChunkWriteLock lock(&chunk);
		chunk.Reset([&](BlockIndex* blockIndices)
		{
			std::memcpy(blockIndices, blocks.data(), blocks.size() * sizeof(BlockIndex));
		});
	}
}
