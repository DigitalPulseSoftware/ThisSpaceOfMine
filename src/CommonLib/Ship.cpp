// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Ship.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/GameConstants.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Ship::Ship(float tileSize) :
	ChunkContainer(tileSize),
	m_upDirection(Nz::Vector3f::Up())
	{
	}

	FlatChunk& Ship::AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_unique<FlatChunk>(*this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		if (initCallback)
			chunkData.chunk->Reset(initCallback);

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this](Chunk* chunk)
		{
			OnChunkUpdated(this, chunk);
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& /*indices*/, BlockIndex /*newBlock*/)
		{
			OnChunkUpdated(this, chunk);
		});

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		OnChunkAdded(this, it->second.chunk.get());

		return *it->second.chunk;
	}

	float Ship::ComputeGravityAcceleration(const Nz::Vector3f& /*position*/) const
	{
		return Constants::ShipGravityAcceleration;
	}

	Nz::Vector3f Ship::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		return m_upDirection;
	}

	void Ship::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Ship::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Ship::Generate(const BlockLibrary& blockLibrary)
	{
		constexpr unsigned int freespace = 5;

		FlatChunk& chunk = AddChunk({ 0, 0, 0 });

		BlockIndex hullIndex = blockLibrary.GetBlockIndex("hull");
		if (hullIndex == InvalidBlockIndex)
			return;

		constexpr unsigned int boxSize = 12;
		constexpr unsigned int heightSize = 5;
		Nz::Vector3ui startPos = chunk.GetSize() / 2 - Nz::Vector3ui(boxSize / 2, boxSize / 2, heightSize / 2);

		for (unsigned int z = 0; z < heightSize; ++z)
		{
			for (unsigned int y = 0; y < boxSize; ++y)
			{
				for (unsigned int x = 0; x < boxSize; ++x)
				{
					if (x != 0 && x != boxSize - 1 &&
					    y != 0 && y != boxSize - 1 &&
					    z != 0 && z != heightSize - 1)
					{
						continue;
					}

					if (x == 0 && y == boxSize / 2 && z > 0 && z < heightSize - 1)
						continue;

					chunk.UpdateBlock(startPos + Nz::Vector3ui{ x, y, z }, hullIndex);
				}
			}
		}
	}

	void Ship::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		OnChunkRemove(this, it->second.chunk.get());
		m_chunks.erase(it);
	}
}
