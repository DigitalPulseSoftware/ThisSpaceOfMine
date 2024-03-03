// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Ship.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <PerlinNoise.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Ship::Ship(const BlockLibrary& blockLibrary, const Nz::Vector3ui& gridSize, float tileSize) :
	ChunkContainer(gridSize, tileSize)
	{
		m_chunks.resize(m_chunkCount.x * m_chunkCount.y * m_chunkCount.z);

		SetupChunks(blockLibrary);
	}

	Chunk& Ship::AddChunk(const Nz::Vector3ui& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(!m_chunks[index].chunk);
		m_chunks[index].chunk = std::make_unique<FlatChunk>(*this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		if (initCallback)
			m_chunks[index].chunk->InitBlocks(initCallback);

		m_chunks[index].onUpdated.Connect(m_chunks[index].chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& /*indices*/, BlockIndex /*newBlock*/)
		{
			OnChunkUpdated(this, chunk);
		});

		OnChunkAdded(this, m_chunks[index].chunk.get());

		return *m_chunks[index].chunk;
	}

	void Ship::RemoveChunk(const Nz::Vector3ui& indices)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(m_chunks[index].chunk);

		OnChunkRemove(this, m_chunks[index].chunk.get());

		m_chunks[index].chunk = nullptr;
		m_chunks[index].onUpdated.Disconnect();
	}

	void Ship::SetupChunks(const BlockLibrary& blockLibrary)
	{
		constexpr unsigned int freespace = 5;

		BlockIndex hullIndex = blockLibrary.GetBlockIndex("hull");
		if (hullIndex == InvalidBlockIndex)
			return;

		Chunk& chunk = AddChunk({ 0, 0, 0 });

		constexpr unsigned int boxSize = 5;
		Nz::Vector3ui startPos = chunk.GetSize() / 2 - Nz::Vector3ui(boxSize / 2);

		for (unsigned int z = 0; z < boxSize; ++z)
		{
			for (unsigned int y = 0; y < boxSize; ++y)
			{
				for (unsigned int x = 0; x < boxSize; ++x)
				{
					if (x != 0 && x != boxSize - 1 &&
						y != 0 && y != boxSize - 1 &&
					    z != 0 && z != boxSize - 1)
					{
						continue;
					}

					chunk.UpdateBlock(startPos + Nz::Vector3ui{ x, y, z }, hullIndex);
				}
			}
		}
	}
}
