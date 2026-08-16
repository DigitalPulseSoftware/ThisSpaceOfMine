// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planet.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <PerlinNoise.hpp>
#include <spdlog/spdlog.h>
#include <thread>

namespace tsom
{
	Planet::Planet(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float tileSize) :
	ChunkContainer(blockLibrary, tileSize),
	m_app(app)
	{
	}

	Chunk& Planet::AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
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

	void Planet::AddChunks(const Nz::Vector3ui& chunkCount)
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

	void Planet::ClearChunks()
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

	void Planet::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Planet::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Planet::GenerateChunk(Chunk& chunk, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties)
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

	void Planet::GenerateChunks(Nz::TaskScheduler& taskScheduler, const Nz::Vector3ui& chunkCount, std::string_view scriptName, const std::unordered_map<std::string, EntityProperty>& properties)
	{
		m_chunkGenerators.Clear();
		ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			if (chunk.HasContent())
				return;

			taskScheduler.AddTask([=, this, &chunk, props = properties]
			{
				GenerateChunk(chunk, chunkCount, scriptName, props);
			});
		});
	}

	void Planet::GeneratePlatform(Direction upDirection, const BlockIndices& platformCenter)
	{
		constexpr int platformSize = 15;
		constexpr unsigned int freeHeight = 10;
		const DirectionAxis& dirAxis = s_dirAxis[upDirection];

		BlockIndices coordinates = platformCenter;

		int& xPos = coordinates[dirAxis.rightAxis];
		xPos += -dirAxis.rightDir * platformSize / 2;

		int& yPos = coordinates[dirAxis.upAxis];

		int& zPos = coordinates[dirAxis.forwardAxis];
		zPos += -dirAxis.forwardDir * platformSize / 2;

		BlockIndex borderBlockIndex = m_blockLibrary.GetBlockIndex("copper_block");
		BlockIndex interiorBlockIndex = m_blockLibrary.GetBlockIndex("stone_bricks");

		BlockIndices originalCoordinates = coordinates;
		for (unsigned int y = 0; y < freeHeight; ++y)
		{
			unsigned int startingZ = zPos;
			for (unsigned int z = 0; z < platformSize; ++z)
			{
				unsigned int startingX = xPos;
				for (unsigned int x = 0; x < platformSize; ++x)
				{
					BlockIndex blockIndex;
					if (y != 0)
						blockIndex = EmptyBlockIndex;
					else if (x == 0 || x == platformSize - 1 || z == 0 || z == platformSize - 1)
						blockIndex = borderBlockIndex;
					else
						blockIndex = interiorBlockIndex;

					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = GetChunkIndicesByBlockIndices(coordinates, &innerCoordinates);
					if (Chunk* chunk = GetChunk(chunkIndices))
						chunk->UpdateBlock(innerCoordinates, blockIndex);

					xPos += dirAxis.rightDir;
				}

				xPos = startingX;
				zPos += dirAxis.forwardDir;
			}

			if (yPos == 0 && dirAxis.upDir < 0)
				break;

			yPos += dirAxis.upDir;
			zPos = startingZ;
		}

		// Bottom
		BlockIndex planksBlockIndex = m_blockLibrary.GetBlockIndex("planks");

		coordinates = originalCoordinates;
		for (unsigned int y = 1; /*no cond*/; ++y)
		{
			yPos -= dirAxis.upDir;

			bool hasEmpty = false;

			unsigned int startingZ = zPos;
			for (unsigned int z = 0; z < platformSize; ++z)
			{
				unsigned int startingX = xPos;
				for (unsigned int x = 0; x < platformSize; ++x)
				{
					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = GetChunkIndicesByBlockIndices(coordinates, &innerCoordinates);
					Chunk* chunk = GetChunk(chunkIndices);
					if (!chunk)
						continue;

					xPos += dirAxis.rightDir;

					BlockIndex blockIndex;
					if (y % 3 == 0)
					{
						if (x != 0 && x != platformSize - 1 && z != 0 && z != platformSize - 1)
							continue;
					}
					else
					{
						if (chunk->GetBlockContent(innerCoordinates) != EmptyBlockIndex)
							continue;

						if (x != 0 && x != platformSize - 1 || z != 0 && z != platformSize - 1)
							continue;
					}

					hasEmpty = true;
					chunk->UpdateBlock(innerCoordinates, planksBlockIndex);
				}

				xPos = startingX;
				zPos += dirAxis.forwardDir;
			}

			if (!hasEmpty)
				break;

			zPos = startingZ;
		}
	}

	void Planet::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		Chunk* chunk = it->second.chunk.get();
		OnChunkRemove(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerRemove(this, chunk, layerIndex);

		m_chunks.erase(it);
	}
}
