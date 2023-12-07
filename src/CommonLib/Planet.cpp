// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>
#include <PerlinNoise.hpp>
#include <random>

namespace tsom
{
	Planet::Planet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius, float gravityFactor) :
	ChunkContainer(gridSize, tileSize),
	m_cornerRadius(cornerRadius),
	m_gravityFactor(gravityFactor)
	{
		m_chunks.resize(m_chunkCount.x * m_chunkCount.y * m_chunkCount.z);
	}

	Chunk& Planet::AddChunk(const Nz::Vector3ui& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(!m_chunks[index].chunk);
		m_chunks[index].chunk = std::make_unique<FlatChunk>(indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		if (initCallback)
			m_chunks[index].chunk->InitBlocks(initCallback);

		m_chunks[index].onUpdated.Connect(m_chunks[index].chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& /*indices*/, BlockIndex /*newBlock*/)
		{
			OnChunkUpdated(this, chunk);
		});

		OnChunkAdded(this, m_chunks[index].chunk.get());

		return *m_chunks[index].chunk;
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

	void Planet::GenerateChunks(BlockLibrary& blockLibrary)
	{
		constexpr std::size_t freeSpace = 10;

		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
					AddChunk({ x, y, z });
			}
		}

		std::mt19937 rand(std::random_device{}());
		std::bernoulli_distribution dis(0.9);

		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int y = 0; y < m_gridSize.y; ++y)
			{
				for (unsigned int x = 0; x < m_gridSize.x; ++x)
				{
					unsigned int depth = std::min({
						x,
						y,
						z,
						m_gridSize.x - x - 1,
						m_gridSize.y - y - 1,
						m_gridSize.z - z - 1,
					});

					std::string_view blockType;
					if (depth <= 8)
						blockType = "snow";
					else if (depth <= 10)
						blockType = "dirt";
					else if (depth <= 15)
						blockType = "grass";
					else if (depth <= 20)
						blockType = "dirt";
					else
						blockType = (dis(rand)) ? "stone" : "stone_mossy";

					if (x >= m_gridSize.x / 2 - 2 && x < m_gridSize.x / 2 + 1 &&
					    y >= m_gridSize.y / 2 - 2 && y < m_gridSize.y / 2 + 1)
					{
						blockType = "empty";
					}

					BlockIndex blockIndex = blockLibrary.GetBlockIndex(blockType);
					if (blockIndex != InvalidBlockIndex)
					{
						Nz::Vector3ui innerCoordinates;
						Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
						chunk.UpdateBlock(innerCoordinates, blockIndex);
					}
				}
			}
		}

		siv::PerlinNoise perlin(42);

		constexpr std::size_t heightScale = 30;

		// +X
		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int y = 0; y < m_gridSize.y; ++y)
			{
				double height = perlin.normalizedOctave3D_01(0.0, y * 0.1, z * 0.1, 4);

				std::size_t depth = m_gridSize.x - height * heightScale;
				for (unsigned int x = m_gridSize.x - 1; x > depth; --x)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		// +Y
		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int x = 0; x < m_gridSize.x; ++x)
			{
				double height = perlin.normalizedOctave3D_01(x * 0.1, 0.0, z * 0.1, 4);

				std::size_t depth = m_gridSize.y - height * heightScale;
				for (unsigned int y = m_gridSize.y - 1; y > depth; --y)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		// +Z
		for (unsigned int y = 0; y < m_gridSize.y; ++y)
		{
			for (unsigned int x = 0; x < m_gridSize.x; ++x)
			{
				double height = perlin.normalizedOctave3D_01(x * 0.1, y * 0.1, 0.0, 4);

				std::size_t depth = m_gridSize.z - height * heightScale;
				for (unsigned int z = m_gridSize.z - 1; z > depth; --z)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		// -X
		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int y = 0; y < m_gridSize.y; ++y)
			{
				double height = perlin.normalizedOctave3D_01(m_gridSize.z * 0.1, (m_gridSize.y - y) * 0.1, (m_gridSize.z - z) * 0.1, 4);

				std::size_t depth = height * heightScale;
				for (unsigned int x = 0; x < depth; ++x)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		// -Y
		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int x = 0; x < m_gridSize.x; ++x)
			{
				double height = perlin.normalizedOctave3D_01((m_gridSize.x - x) * 0.1, m_gridSize.y * 0.1, (m_gridSize.z - z) * 0.1, 4);

				std::size_t depth = height * heightScale;
				for (unsigned int y = 0; y < depth; ++y)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		// -Z
		for (unsigned int y = 0; y < m_gridSize.y; ++y)
		{
			for (unsigned int x = 0; x < m_gridSize.x; ++x)
			{
				double height = perlin.normalizedOctave3D_01((m_gridSize.x - x) * 0.1, (m_gridSize.y - y) * 0.1, m_gridSize.z * 0.1, 4);

				std::size_t depth = height * heightScale;
				for (unsigned int z = 0; z < depth; ++z)
				{
					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, EmptyBlockIndex);
				}
			}
		}

		/*for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int y = 0; y < m_gridSize.y; ++y)
			{
				for (unsigned int x = 0; x < m_gridSize.x; ++x)
				{
					if (x < freeSpace || x >= m_gridSize.x - freeSpace ||
						y < freeSpace || y >= m_gridSize.y - freeSpace ||
						z < freeSpace || z >= m_gridSize.z - freeSpace)
						continue;

					unsigned int depth = std::min({
						x - freeSpace,
						y - freeSpace,
						z - freeSpace,
						m_gridSize.x - freeSpace - x - 1,
						m_gridSize.y - freeSpace - y - 1,
						m_gridSize.z - freeSpace - z - 1,
					});

					VoxelBlock blockType;
					if (depth == 0)
						blockType = VoxelBlock::Grass;
					else if (depth <= 3)
						blockType = VoxelBlock::Dirt;
					else
						blockType = VoxelBlock::Stone;

					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, blockType);
				}
			}
		}*/

		/*for (auto& gridVec : m_grids)
		{
			gridVec.clear();

			std::size_t maxHeight = m_gridDimensions / 2;

			std::size_t gridSize = 1;
			for (std::size_t i = 0; i < m_gridDimensions; ++i)
			{
				VoxelBlock cell;
				if (i >= maxHeight)
					cell = EmptyBlockIndex;
				else if (i == maxHeight - 1)
					cell = VoxelBlock::Grass;
				else if (i >= maxHeight - 3)
					cell = VoxelBlock::Dirt;
				else
					cell = VoxelBlock::Stone;

				gridVec.emplace_back(std::make_unique<Chunk>(gridSize, gridSize, cell));
				gridSize += 2;
			}
		}*/
	}

	void Planet::RemoveChunk(const Nz::Vector3ui& indices)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(m_chunks[index].chunk);

		OnChunkRemove(this, m_chunks[index].chunk.get());

		m_chunks[index].chunk = nullptr;
		m_chunks[index].onUpdated.Disconnect();
	}
}
