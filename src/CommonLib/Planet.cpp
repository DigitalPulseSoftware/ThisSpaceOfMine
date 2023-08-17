// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
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
	Planet::Planet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius) :
	m_chunkCount(Nz::Vector3ui((gridSize + Nz::Vector3ui(ChunkSize - 1)) / ChunkSize)),
	m_gridSize(gridSize),
	m_cornerRadius(cornerRadius),
	m_tileSize(tileSize)
	{
		m_chunks.resize(m_chunkCount.x * m_chunkCount.y * m_chunkCount.z);
	}

	Chunk& Planet::AddChunk(const Nz::Vector3ui& indices, const Nz::FunctionRef<void(VoxelBlock* blocks)>& initCallback)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(!m_chunks[index].chunk);
		m_chunks[index].chunk = std::make_unique<FlatChunk>(indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		if (initCallback)
			m_chunks[index].chunk->InitBlocks(initCallback);

		m_chunks[index].onUpdated.Connect(m_chunks[index].chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& /*indices*/, VoxelBlock /*newBlock*/)
		{
			OnChunkUpdated(this, chunk);
		});

		OnChunkAdded(this, m_chunks[index].chunk.get());

		return *m_chunks[index].chunk;
	}

	void Planet::GenerateChunks()
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

					VoxelBlock blockType;
					if (depth <= 5)
						blockType = VoxelBlock::Snow;
					else if (depth <= 10)
						blockType = VoxelBlock::Dirt;
					else if (depth <= 15)
						blockType = VoxelBlock::Grass;
					else if (depth <= 40)
						blockType = VoxelBlock::Dirt;
					else
						blockType = VoxelBlock::Stone;


					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateBlock(innerCoordinates, blockType);
				}
			}
		}

		siv::PerlinNoise perlin(42);

		constexpr std::size_t heightScale = 20;

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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Empty);
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
					cell = VoxelBlock::Empty;
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
