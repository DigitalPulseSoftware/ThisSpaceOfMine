// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planet.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <Nazara/Core/VertexStruct.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <PerlinNoise.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Planet::Planet(float tileSize, float cornerRadius, float gravity) :
	ChunkContainer(tileSize),
	m_cornerRadius(cornerRadius),
	m_gravity(gravity)
	{
	}

	Chunk& Planet::AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_unique<FlatChunk>(*this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this](Chunk* chunk)
		{
			OnChunkUpdated(this, chunk, DirectionMask_All);
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex /*newBlock*/)
		{
			DirectionMask neighborMask;
			if (indices.x == 0)
				neighborMask |= Direction::Left;
			else if (indices.x == chunk->GetSize().x - 1)
				neighborMask |= Direction::Right;

			if (indices.y == 0)
				neighborMask |= Direction::Front;
			else if (indices.y == chunk->GetSize().y - 1)
				neighborMask |= Direction::Back;

			if (indices.z == 0)
				neighborMask |= Direction::Up;
			else if (indices.z == chunk->GetSize().z - 1)
				neighborMask |= Direction::Down;

			OnChunkUpdated(this, chunk, neighborMask);
		});

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		OnChunkAdded(this, it->second.chunk.get());

		if (initCallback)
			it->second.chunk->Reset(initCallback);

		return *it->second.chunk;
	}

	float Planet::ComputeGravityAcceleration(const Nz::Vector3f& position) const
	{
		if (position.SquaredDistance(GetCenter()) < Nz::IntegralPow(10.f, 2))
			return 0.f;

		return m_gravity;
	}

	Nz::Vector3f Planet::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		Nz::Vector3f center = GetCenter();

		float distToCenter = std::max({
			std::abs(position.x - center.x),
			std::abs(position.y - center.y),
			std::abs(position.z - center.z),
		});

		float innerReductionSize = std::max(distToCenter - std::max(m_cornerRadius, 1.f), 0.f);
		Nz::Boxf innerBox(center - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());

		return Nz::Vector3f::Normalize(position - innerPos);
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

	void Planet::GenerateChunk(const BlockLibrary& blockLibrary, Chunk& chunk, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount)
	{
		constexpr std::size_t freeSpace = 30;

		ChunkIndices chunkIndices = chunk.GetIndices();
		Nz::UInt32 chunkSeed = seed + static_cast<Nz::UInt32>(chunkIndices.x) + static_cast<Nz::UInt32>(chunkIndices.y) + static_cast<Nz::UInt32>(chunkIndices.z);

		std::minstd_rand rand(chunkSeed);
		std::bernoulli_distribution dis(0.9);

		BlockIndex dirtBlockIndex = blockLibrary.GetBlockIndex("dirt");
		BlockIndex grassBlockIndex = blockLibrary.GetBlockIndex("grass");
		BlockIndex stoneBlockIndex = blockLibrary.GetBlockIndex("stone");
		BlockIndex stoneMossyBlockIndex = blockLibrary.GetBlockIndex("stone_mossy");
		BlockIndex snowBlockIndex = blockLibrary.GetBlockIndex("snow");

		Nz::Vector3i maxHeight((Nz::Vector3i(chunkCount) + Nz::Vector3i(1)) / 2);
		maxHeight *= int(Planet::ChunkSize);

		Nz::EnumArray<Direction, siv::PerlinNoise> perlin;
		for (auto&& [dir, noise] : perlin.iter_kv())
			noise.reseed(seed + static_cast<unsigned int>(dir));

		chunk.LockWrite();
		NAZARA_DEFER({ chunk.UnlockWrite(); });

		chunk.Reset([&](BlockIndex* blockIndices)
		{
			// Fill all blocks based on their depth
			ChunkIndices chunkIndices = chunk.GetIndices();

			BlockIndex* blockIndexPtr = blockIndices;
			for (unsigned int z = 0; z < Planet::ChunkSize; ++z)
			{
				for (unsigned int y = 0; y < Planet::ChunkSize; ++y)
				{
					for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
					{
						Nz::Vector3i blockPos = GetBlockIndices(chunkIndices, { x, y, z });
						unsigned int depth = Nz::SafeCaster(std::min({
							maxHeight.x - std::abs(blockPos.x),
							maxHeight.y - std::abs(blockPos.z),
							maxHeight.z - std::abs(blockPos.y)
						}));

						if (depth < freeSpace)
						{
							*blockIndexPtr++ = EmptyBlockIndex;
							continue;
						}

						depth -= freeSpace;

						BlockIndex blockIndex;
						if (depth <= 6)
							blockIndex = snowBlockIndex;
						else if (depth <= 18)
							blockIndex = dirtBlockIndex;
						else
							blockIndex = (dis(rand)) ? stoneBlockIndex : stoneMossyBlockIndex;

						if (std::abs(blockPos.x) <= 2 && std::abs(blockPos.z) <= 2)
							blockIndex = EmptyBlockIndex;

						if (blockIndex != InvalidBlockIndex)
							*blockIndexPtr++ = blockIndex;
					}
				}
			}

			constexpr double heightScale = 1.5f;
			constexpr double scale = 0.02f;

			// +X
			for (unsigned int y = 0; y < Planet::ChunkSize; ++y)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { 0, x, y });
					double height = perlin[Direction::Right].normalizedOctave2D_01(mapPos.y * scale, mapPos.z * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.x / 2 - freeSpace) + freeSpace, maxHeight.x / 2));
					int blockDepth = maxHeight.x - mapPos.x + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCaster(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ startHeight, x, y })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ height, x, y })] = EmptyBlockIndex;
				}
			}

			// -X
			for (unsigned int y = 0; y < Planet::ChunkSize; ++y)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { Planet::ChunkSize - 1, x, y });
					double height = perlin[Direction::Left].normalizedOctave2D_01(mapPos.y * scale, mapPos.z * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.x / 2 - freeSpace) + freeSpace, maxHeight.x / 2));
					int blockDepth = maxHeight.x + mapPos.x + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCast<unsigned int>(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ Planet::ChunkSize - startHeight - 1, x, y })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ Planet::ChunkSize - height - 1, x, y })] = EmptyBlockIndex;
				}
			}

			// +Y
			for (unsigned int z = 0; z < Planet::ChunkSize; ++z)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { x, z, 0 });
					double height = perlin[Direction::Up].normalizedOctave2D_01(mapPos.x * scale, mapPos.z * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.y / 2 - freeSpace) + freeSpace, maxHeight.y / 2));
					int blockDepth = maxHeight.y - mapPos.y + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCaster(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ x, z, startHeight })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ x, z, height })] = EmptyBlockIndex;
				}
			}

			// -Y
			for (unsigned int z = 0; z < Planet::ChunkSize; ++z)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { x, z, Planet::ChunkSize - 1 });
					double height = perlin[Direction::Down].normalizedOctave2D_01(mapPos.x * scale, mapPos.z * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.y / 2 - freeSpace) + freeSpace, maxHeight.y / 2));
					int blockDepth = maxHeight.y + mapPos.y + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCast<unsigned int>(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ x, z, Planet::ChunkSize - startHeight - 1 })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ x, z, Planet::ChunkSize - height - 1 })] = EmptyBlockIndex;
				}
			}

			// +Z
			for (unsigned int y = 0; y < Planet::ChunkSize; ++y)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { x, 0, y });
					double height = perlin[Direction::Back].normalizedOctave2D_01(mapPos.x * scale, mapPos.y * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.z / 2 - freeSpace) + freeSpace, maxHeight.z / 2));
					int blockDepth = maxHeight.z - mapPos.z + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCaster(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ x, startHeight, y })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ x, height, y })] = EmptyBlockIndex;
				}
			}

			// -Z
			for (unsigned int y = 0; y < Planet::ChunkSize; ++y)
			{
				for (unsigned int x = 0; x < Planet::ChunkSize; ++x)
				{
					BlockIndices mapPos = GetBlockIndices(chunkIndices, { x, Planet::ChunkSize - 1, y });
					double height = perlin[Direction::Front].normalizedOctave2D_01(mapPos.x * scale, mapPos.y * scale, 4) * heightScale;

					int terrainDepth = std::round(std::min<double>(height * (maxHeight.z / 2 - freeSpace) + freeSpace, maxHeight.z / 2));
					int blockDepth = maxHeight.z + mapPos.z + 1;
					if (blockDepth < terrainDepth)
						continue;

					unsigned int startHeight = Nz::SafeCaster(blockDepth - terrainDepth);
					if (startHeight >= Planet::ChunkSize)
						continue;

					if (BlockIndex& blockType = blockIndices[chunk.GetBlockLocalIndex({ x, Planet::ChunkSize - startHeight - 1, y })]; blockType == dirtBlockIndex)
						blockType = grassBlockIndex;

					for (unsigned int height = startHeight + 1; height < Planet::ChunkSize; ++height)
						blockIndices[chunk.GetBlockLocalIndex({ x, Planet::ChunkSize - height - 1, y })] = EmptyBlockIndex;
				}
			}
		});
	}

	void Planet::GenerateChunks(const BlockLibrary& blockLibrary, Nz::TaskScheduler& taskScheduler, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount)
	{
		for (int chunkZ = 0; chunkZ < chunkCount.z; ++chunkZ)
		{
			for (int chunkY = 0; chunkY < chunkCount.y; ++chunkY)
			{
				for (int chunkX = 0; chunkX < chunkCount.x; ++chunkX)
				{
					auto& chunk = AddChunk({ chunkX - int(chunkCount.x / 2), chunkY - int(chunkCount.y / 2), chunkZ - int(chunkCount.z / 2) });
					taskScheduler.AddTask([&]
					{
						GenerateChunk(blockLibrary, chunk, seed, chunkCount);
					});
				}
			}
		}

		taskScheduler.WaitForTasks();
	}

	void Planet::GeneratePlatform(const BlockLibrary& blockLibrary, Direction upDirection, const BlockIndices& platformCenter)
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

		BlockIndex borderBlockIndex = blockLibrary.GetBlockIndex("copper_block");
		BlockIndex interiorBlockIndex = blockLibrary.GetBlockIndex("stone_bricks");

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
		BlockIndex planksBlockIndex = blockLibrary.GetBlockIndex("planks");

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

		OnChunkRemove(this, it->second.chunk.get());
		m_chunks.erase(it);
	}
}
