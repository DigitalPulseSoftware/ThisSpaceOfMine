// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Chunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Math/Box.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <cassert>
#include <numeric>

namespace tsom
{
	Chunk::~Chunk() = default;

	void Chunk::BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace) const
	{
		auto DrawFace = [&](BlockIndex blockContent, const Nz::Vector3ui& blockIndices, Direction direction, const Nz::Vector3f& blockCenter, const std::array<Nz::Vector3f, 4>& pos)
		{
			VertexAttributes vertexAttributes = addFace(blockIndices, direction);
			assert(vertexAttributes.position);

			indices.push_back(vertexAttributes.firstIndex);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 1);

			indices.push_back(vertexAttributes.firstIndex + 1);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 3);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			Nz::Vector3f faceCenter = std::accumulate(pos.begin(), pos.end(), Nz::Vector3f::Zero()) / pos.size();
			Nz::Vector3f faceDirection = Nz::Vector3f::Normalize(faceCenter - blockCenter);

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = faceDirection;
			}

			if (vertexAttributes.blockIndex)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.blockIndex[i] = blockContent;
			}
		};

		Nz::EnumArray<Direction, const Chunk*> neighborChunks;
		MultiChunkReadLock chunkLock;
		for (auto&& [dir, chunk] : neighborChunks.iter_kv())
		{
			chunk = m_owner.GetChunk(m_indices + s_chunkDirOffset[dir]);
			if (chunk)
				chunkLock.AddChunk(chunk);
		}

		chunkLock.Lock();

		auto GetNeighborBlock = [&](Nz::Vector3ui indices, Direction direction) -> std::optional<BlockIndex>
		{
			ChunkIndices chunkIndices = m_indices;
			std::swap(chunkIndices.y, chunkIndices.z);

			for (unsigned int axis : { 0, 1, 2 })
			{
				unsigned int& index = indices[axis];
				int offset = s_blockDirOffset[direction][axis];
				assert(offset >= -1 && offset <= 1);

				if (offset > 0)
				{
					index += offset;
					if (index >= m_size[axis])
					{
						index -= m_size[axis];
						chunkIndices[axis]++;
					}
				}
				else if (offset < 0)
				{
					unsigned int posOffset = std::abs(offset);
					if (posOffset > index)
					{
						index += m_size[axis];
						chunkIndices[axis]--;
					}

					index -= posOffset;
				}
			}

			std::swap(chunkIndices.y, chunkIndices.z);

			if (chunkIndices != m_indices)
			{
				const Chunk* chunk = neighborChunks[direction];
				if (!chunk)
					return {};

				if (!chunk->HasContent())
					return {};

				return chunk->GetBlockContent(indices);
			}
			else
				return GetBlockContent(indices);
		};

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					Nz::Vector3ui blockIndices(x, y, z);

					BlockIndex blockIndex = GetBlockContent(blockIndices);
					if (blockIndex == EmptyBlockIndex)
						continue;

					const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);
					if (blockData.layerIndex != layerIndex)
						continue;

					// Get unaltered voxel corners and deform them next
					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = Chunk::ComputeBlockCorners(blockIndices);

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					auto IsTransparent = [&](BlockIndex neighborBlockIndex)
					{
						// don't render faces between blocks of the same type even if transparent
						if (blockIndex == neighborBlockIndex)
							return false;

						const auto& neighborBlockData = m_blockLibrary.GetBlockData(neighborBlockIndex);
						return neighborBlockData.isTransparent;
					};

					// Up
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Up); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Up, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Up, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear] });
					}

					// Down
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Down); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Down, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Down, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
					}

					// Front
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Front); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Front, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::RightBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Front, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Back
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Back); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Back, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Back, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::LeftBottomNear] });
					}

					// Left
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Left); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Left, blockCenter, { corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Left, blockCenter, { corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Right
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Right); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Right, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Right, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar] });
					}
				}
			}
		}
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> Chunk::ComputeBlockCorners(const Nz::Vector3ui& indices) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize;
		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		return box.GetCorners();
	}

	void Chunk::Deserialize(Nz::ByteStream& byteStream)
	{
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

			BlockIndex blockIndex = m_blockLibrary.GetBlockIndex(blockName);
			if (blockIndex == InvalidBlockIndex)
				throw std::runtime_error("unknown block " + blockName);

			deserializationIndices.push_back(blockIndex);
		}

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
			else
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

	void Chunk::Serialize(Nz::ByteStream& byteStream) const
	{
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

			byteStream << m_blockLibrary.GetBlockData(i).name;
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

	void Chunk::UpdateBlock(const Nz::Vector3ui& indices, BlockIndex newBlock, bool ensureContent)
	{
		if (ensureContent && !HasContent())
			Reset();

		NazaraAssertMsg(HasContent(), "chunk has not been reset");

		const auto& newBlockData = m_blockLibrary.GetBlockData(newBlock);

		unsigned int blockIndex = GetBlockLocalIndex(indices);
		BlockIndex oldBlock = m_blocks[blockIndex];
		const auto& oldBlockData = m_blockLibrary.GetBlockData(oldBlock);
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

		OnBlockUpdated(this, indices, oldBlock, newBlock, oldBlockData.layerIndex, newBlockData.layerIndex);

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
			auto& previousBlockData = m_blockLibrary.GetBlockData(oldBlock);
			auto& newBlockData = m_blockLibrary.GetBlockData(newBlock);

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

		// Unregister layer only after update to avoid triggering chunk update (OnBlockUpodated)
		assert(m_layers[oldBlockData.layerIndex]->blockCount > 0);
		if (--m_layers[oldBlockData.layerIndex]->blockCount == 0)
			UnregisterLayer(oldBlockData.layerIndex);
	}

	void Chunk::OnChunkReset()
	{
		std::fill(m_blockTypeCount.begin(), m_blockTypeCount.end(), 0);

		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			BlockIndex blockContent = m_blocks[blockIndex];
			const auto& blockData = m_blockLibrary.GetBlockData(blockContent);

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

		// Rebuild visibility mask
		m_directionHoleCount.fill(0);

		// Left / right
		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				BlockIndex leftBlockIndex = GetBlockContent({ 0, y, z });
				BlockIndex rightBlockIndex = GetBlockContent({ m_size.x - 1, y, z });

				auto& leftBlockData = m_blockLibrary.GetBlockData(leftBlockIndex);
				if (leftBlockData.isTransparent)
					m_directionHoleCount[Direction::Left]++;

				auto& rightBlockData = m_blockLibrary.GetBlockData(rightBlockIndex);
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

				auto& frontBlockData = m_blockLibrary.GetBlockData(frontBlockIndex);
				if (frontBlockData.isTransparent)
					m_directionHoleCount[Direction::Front]++;

				auto& backBlockData = m_blockLibrary.GetBlockData(backBlockIndex);
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

				auto& downBlockData = m_blockLibrary.GetBlockData(downBlockIndex);
				if (downBlockData.isTransparent)
					m_directionHoleCount[Direction::Down]++;

				auto& upBlockData = m_blockLibrary.GetBlockData(upBlockIndex);
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

		OnReset(this);

		if (m_visibilityMask != oldVisibilityMask)
			OnVisibilityMaskUpdated(this, oldVisibilityMask, m_visibilityMask);
	}
}
