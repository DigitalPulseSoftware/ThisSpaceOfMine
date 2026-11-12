// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Chunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Math/Box.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <spdlog/spdlog.h>
#include <cassert>

namespace tsom
{
	Chunk::~Chunk() = default;

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> Chunk::ComputeBlockCorners(const Nz::Vector3ui& indices) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize;
		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		return box.GetCorners();
	}

	void Chunk::Deserialize(Nz::ByteStream& byteStream)
	{
		const auto& blockLibrary = m_owner.GetBlockLibrary();

		Nz::UInt32 chunkBinaryVersion;
		byteStream >> chunkBinaryVersion;

		if (chunkBinaryVersion != Constants::ChunkBinaryVersion)
			throw std::runtime_error("incompatible chunk version");

		Nz::Vector3ui chunkSize;
		byteStream >> chunkSize;

		if (chunkSize != m_size)
			throw std::runtime_error("incompatible chunk size");

		std::vector<BlockIndex> deserializationIndices;

		Nz::UInt16 blockTypeCount;
		byteStream >> blockTypeCount;

		deserializationIndices.reserve(blockTypeCount);

		std::string blockName;
		for (Nz::UInt16 i = 0; i < blockTypeCount; ++i)
		{
			byteStream >> blockName;

			BlockIndex blockIndex = blockLibrary.GetBlockIndex(blockName);
			if (blockIndex == InvalidBlockIndex)
			{
				spdlog::error("chunk loading: unknown block {}", blockName);
				blockIndex = EmptyBlockIndex;
			}

			deserializationIndices.push_back(blockIndex);
		}

		if (blockTypeCount > 0)
		{
			Reset([&](BlockIndex* blockIndices)
			{
				std::size_t blockCount = GetBlockCount();
				if (blockTypeCount > 8)
				{
					for (std::size_t i = 0; i < blockCount; ++i)
					{
						Nz::UInt16 value;
						byteStream >> value;

						*blockIndices++ = deserializationIndices[value];
					}
				}
				else if (blockTypeCount > 0)
				{
					for (std::size_t i = 0; i < blockCount; ++i)
					{
						Nz::UInt8 value;
						byteStream >> value;

						*blockIndices++ = deserializationIndices[value];
					}
				}
			});
		}
		else
			ClearContent();
	}

	void Chunk::Serialize(Nz::ByteStream& byteStream) const
	{
		const auto& blockLibrary = m_owner.GetBlockLibrary();

		byteStream << Constants::ChunkBinaryVersion;
		byteStream << m_size;

		std::vector<BlockIndex> serializationIndices(m_blockTypeCount.size());
		Nz::UInt16 nextUniqueIndex = 0;

		for (BlockIndex i = 0; i < m_blockTypeCount.size(); ++i)
		{
			if (m_blockTypeCount[i] == 0)
				continue;

			serializationIndices[i] = nextUniqueIndex++;
		}

		byteStream << Nz::SafeCast<Nz::UInt16>(nextUniqueIndex);
		for (BlockIndex i = 0; i < m_blockTypeCount.size(); ++i)
		{
			if (m_blockTypeCount[i] == 0)
				continue;

			byteStream << blockLibrary.GetBlockData(i).name;
		}

		// nextUniqueIndex is the number of bits required to store all the different block types used
		if (nextUniqueIndex > 8)
		{
			for (BlockIndex blockIndex : m_blocks)
				byteStream << static_cast<Nz::UInt16>(serializationIndices[blockIndex]);
		}
		else
		{
			for (BlockIndex blockIndex : m_blocks)
				byteStream << static_cast<Nz::UInt8>(serializationIndices[blockIndex]);
		}
	}

	void Chunk::UpdateBlock(const Nz::Vector3ui& indices, BlockIndex newBlock)
	{
		if (!HasContent())
		{
			if (newBlock == EmptyBlockIndex)
				return;

			PrepareForContent();
		}

		const auto& blockLibrary = m_owner.GetBlockLibrary();
		const auto& newBlockData = blockLibrary.GetBlockData(newBlock);

		unsigned int blockIndex = GetBlockLocalIndex(indices);
		BlockIndex oldBlock = m_blocks[blockIndex];
		const auto& oldBlockData = blockLibrary.GetBlockData(oldBlock);
		assert(IsLayerRegistered(oldBlockData.layerIndex));
		m_blocks[blockIndex] = newBlock;

		if (!IsLayerRegistered(newBlockData.layerIndex))
			RegisterLayer(newBlockData.layerIndex);

		m_layers[oldBlockData.layerIndex]->collisionCellMasks[blockIndex] = false;
		m_layers[newBlockData.layerIndex]->collisionCellMasks[blockIndex] = newBlockData.hasCollisions;
		m_layers[newBlockData.layerIndex]->blockCount++;

		m_blockTypeCount[oldBlock]--;
		if (newBlock >= m_blockTypeCount.size())
			m_blockTypeCount.resize(newBlock + 1);

		m_blockTypeCount[newBlock]++;

		// Check if the chunk is made of only empty blocks and reclaim its memory
		if (!m_isBatchUpdating && GetBlockCount(EmptyBlockIndex) == GetBlockCount())
		{
			ClearContent();
			return;
		}

		OnBlockUpdated(this, indices, oldBlock, newBlock, oldBlockData.layerIndex, newBlockData.layerIndex);

		if (!m_isBatchUpdating)
		{
			// Update visibility mask
			DirectionMask directionMask;
			if (indices.x == 0)
				directionMask |= Direction::Left;
			else if (indices.x == m_size.x - 1)
				directionMask |= Direction::Right;

			if (indices.y == 0)
				directionMask |= Direction::Front;
			else if (indices.y == m_size.y - 1)
				directionMask |= Direction::Back;

			if (indices.z == 0)
				directionMask |= Direction::Down;
			else if (indices.z == m_size.z - 1)
				directionMask |= Direction::Up;

			if (directionMask != 0)
			{
				auto& previousBlockData = blockLibrary.GetBlockData(oldBlock);
				auto& newBlockData = blockLibrary.GetBlockData(newBlock);

				if (previousBlockData.isTransparent != newBlockData.isTransparent)
				{
					DirectionMask oldVisibilityMask = m_visibilityMask;

					if (previousBlockData.isTransparent)
					{
						// We're putting an opaque block on a transparent one
						for (Direction direction : directionMask)
						{
							NazaraAssert(m_directionHoleCount[direction] > 0);
							if (--m_directionHoleCount[direction] == 0)
								m_visibilityMask.Clear(direction);
						}
					}
					else
					{
						// Replacing an opaque block by a transparent one
						for (Direction direction : directionMask)
						{
							m_directionHoleCount[direction]++;
							m_visibilityMask |= direction;
						}
					}

					if (oldVisibilityMask != m_visibilityMask)
						OnVisibilityMaskUpdated(this, oldVisibilityMask, m_visibilityMask);
				}
			}
		}

		// Unregister layer only after update to avoid triggering chunk update (OnBlockUpdated)
		assert(m_layers[oldBlockData.layerIndex]->blockCount > 0);
		if (--m_layers[oldBlockData.layerIndex]->blockCount == 0)
			UnregisterLayer(oldBlockData.layerIndex);
	}

	void Chunk::OnChunkReset()
	{
		assert(HasContent());

		std::fill(m_blockTypeCount.begin(), m_blockTypeCount.end(), 0);

		const auto& blockLibrary = m_owner.GetBlockLibrary();

		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			BlockIndex blockContent = m_blocks[blockIndex];
			const auto& blockData = blockLibrary.GetBlockData(blockContent);

			if NAZARA_UNLIKELY(!IsLayerRegistered(blockData.layerIndex))
			{
				// First block on this layer, register it
				RegisterLayer(blockData.layerIndex);
			}

			m_layers[blockData.layerIndex]->blockCount++;
			m_layers[blockData.layerIndex]->collisionCellMasks[blockIndex] = blockData.hasCollisions;

			if (blockContent >= m_blockTypeCount.size())
				m_blockTypeCount.resize(blockContent + 1);

			m_blockTypeCount[blockContent]++;
		}

		// Check if the chunk is made of only empty blocks and reclaim its memory
		if (GetBlockCount(EmptyBlockIndex) == GetBlockCount())
		{
			ClearContent();
			return;
		}

		OnReset(this);

		RebuildVisibilityMask();
	}

	void Chunk::RebuildVisibilityMask()
	{
		m_directionHoleCount.fill(0);

		const auto& blockLibrary = m_owner.GetBlockLibrary();

		// Left / right
		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				BlockIndex leftBlockIndex = GetBlockContent({ 0, y, z });
				BlockIndex rightBlockIndex = GetBlockContent({ m_size.x - 1, y, z });

				auto& leftBlockData = blockLibrary.GetBlockData(leftBlockIndex);
				if (leftBlockData.isTransparent)
					m_directionHoleCount[Direction::Left]++;

				auto& rightBlockData = blockLibrary.GetBlockData(rightBlockIndex);
				if (rightBlockData.isTransparent)
					m_directionHoleCount[Direction::Right]++;
			}
		}

		// Front / back
		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int x = 0; x < m_size.x; ++x)
			{
				BlockIndex frontBlockIndex = GetBlockContent({ x, 0, z });
				BlockIndex backBlockIndex = GetBlockContent({ x, m_size.y - 1, z });

				auto& frontBlockData = blockLibrary.GetBlockData(frontBlockIndex);
				if (frontBlockData.isTransparent)
					m_directionHoleCount[Direction::Front]++;

				auto& backBlockData = blockLibrary.GetBlockData(backBlockIndex);
				if (backBlockData.isTransparent)
					m_directionHoleCount[Direction::Back]++;
			}
		}

		// Down / up
		for (unsigned int y = 0; y < m_size.y; ++y)
		{
			for (unsigned int x = 0; x < m_size.x; ++x)
			{
				BlockIndex downBlockIndex = GetBlockContent({ x, y, 0 });
				BlockIndex upBlockIndex = GetBlockContent({ x, y, m_size.z - 1 });

				auto& downBlockData = blockLibrary.GetBlockData(downBlockIndex);
				if (downBlockData.isTransparent)
					m_directionHoleCount[Direction::Down]++;

				auto& upBlockData = blockLibrary.GetBlockData(upBlockIndex);
				if (upBlockData.isTransparent)
					m_directionHoleCount[Direction::Up]++;
			}
		}

		DirectionMask oldVisibilityMask = m_visibilityMask;
		m_visibilityMask.Clear();

		for (auto&& [direction, holeCount] : m_directionHoleCount.iter_kv())
		{
			if (holeCount > 0)
				m_visibilityMask |= direction;
		}

		if (m_visibilityMask != oldVisibilityMask)
			OnVisibilityMaskUpdated(this, oldVisibilityMask, m_visibilityMask);
	}
}
