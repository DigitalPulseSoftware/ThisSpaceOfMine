// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Chunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Math/Box.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/MathUtils.hpp>
#include <cassert>
#include <numeric>

namespace tsom
{
	Chunk::Chunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize) :
	m_size(size),
	m_indices(indices),
	m_blockLibrary(blockLibrary),
	m_owner(owner),
	m_hasPerFaceCollision(false),
	m_blockSize(cellSize)
	{
		assert(m_size.x <= Constants::MaxChunkSize);
		assert(m_size.y <= Constants::MaxChunkSize);
		assert(m_size.z <= Constants::MaxChunkSize);
	}

	Chunk::~Chunk() = default;

	void Chunk::BuildFaces(std::size_t layerIndex, const Nz::FunctionRef<void(BlockIndex blockContent, const Nz::Vector3ui& blockIndices, Direction direction)>& addFace, bool ignoreDoubleSided, bool debugClipFace /*= false*/) const
	{
		// Find and lock all neighbor chunks to avoid discrepancies between chunks
		Nz::EnumArray<Direction, const Chunk*> neighborChunks;

		if (!debugClipFace)
		{
			for (auto&& [dir, chunk] : neighborChunks.iter_kv())
			{
				chunk = m_owner.GetChunk(m_indices + s_chunkDirOffset[dir]);
				if (!chunk)
					continue;

				chunk->LockRead();
			}
		}
		else
			neighborChunks.fill(nullptr);

		NAZARA_DEFER(
		{
			for (const Chunk* chunk : neighborChunks)
			{
				if (chunk)
					chunk->UnlockRead();
			}
		});

		assert(m_size.x == m_size.y && m_size.x == m_size.z);
		unsigned int chunkSize = m_size.x;
		unsigned int chunkSizeSquared = chunkSize * chunkSize;
		std::vector<Nz::UInt32> opaqueBlockMask(chunkSizeSquared * 3, 0); // X | Y | Z

		constexpr std::array<Direction, 6> axisDir = {
			Direction::Left,  Direction::Right,
			Direction::Front, Direction::Back,
			Direction::Down,  Direction::Up
		};

		for (unsigned int z = 0; z < chunkSize; ++z)
		{
			for (unsigned int y = 0; y < chunkSize; ++y)
			{
				for (unsigned int x = 0; x < chunkSize; ++x)
				{
					Nz::Vector3ui blockIndices(x, y, z);

					BlockIndex blockIndex = GetBlockContent(blockIndices);
					//unsigned int l = blockIndices.z % 2;
					//BlockIndex blockIndex = (blockIndices.x % 2 == l || blockIndices.y % 2 != l) ? EmptyBlockIndex : 1;
					const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);
					if (blockData.layerIndex != layerIndex)
						continue;

					opaqueBlockMask[z * chunkSize + y] |= ((!blockData.isTransparent) ? 1u : 0u) << (x + 1);
					opaqueBlockMask[chunkSizeSquared + z * chunkSize + x] |= ((!blockData.isTransparent) ? 1u : 0u) << (y + 1);
					opaqueBlockMask[2 * chunkSizeSquared + y * chunkSize + x] |= ((!blockData.isTransparent) ? 1u : 0u) << (z + 1);
				}
			}
		}

		auto GetBlockIndices = [](unsigned int axis, unsigned int i, unsigned int j, unsigned int k)
		{
			std::array axisIndices = {
				Nz::Vector3ui{ k, j, i },
				Nz::Vector3ui{ j, k, i },
				Nz::Vector3ui{ j, i, k }
			};

			return axisIndices[axis];
		};

		// Fill with neighbor chunks
#if 0
		for (unsigned int axis : { 0, 1, 2 })
		{
			const Chunk* leftChunk = neighborChunks[axisDir[axis * 2]];
			if (leftChunk && leftChunk->HasContent())
			{
				Nz::UInt32* opaqueVoxelMaskPtr = &opaqueBlockMask[axis * chunkSizeSquared];
				for (unsigned int i = 0; i < chunkSize; ++i)
				{
					for (unsigned int j = 0; j < chunkSize; ++j)
					{
						BlockIndex blockIndex = leftChunk->GetBlockContent(GetBlockIndices(axis, i, j, chunkSize - 1));
						const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

						opaqueVoxelMaskPtr[i * chunkSize + j] |= ((!blockData.isTransparent) ? 1u : 0u) << (chunkSize - 1);
					}
				}
			}

			const Chunk* rightChunk = neighborChunks[axisDir[axis * 2 + 1]];
			if (rightChunk && rightChunk->HasContent())
			{
				Nz::UInt32* opaqueVoxelMaskPtr = &opaqueBlockMask[axis * chunkSizeSquared];
				for (unsigned int i = 0; i < chunkSize; ++i)
				{
					for (unsigned int j = 0; j < chunkSize; ++j)
					{
						BlockIndex blockIndex = rightChunk->GetBlockContent(GetBlockIndices(axis, i, j, 0));
						const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

						opaqueVoxelMaskPtr[i * chunkSize + j] |= ((!blockData.isTransparent) ? 1u : 0u) << 0;
					}
				}
			}
		}
#endif

		auto AddFace = [&](const Nz::Vector3ui& blockIndices, Direction direction)
		{
			BlockIndex blockIndex = GetBlockContent(blockIndices);
			addFace(blockIndex, blockIndices, direction);
		};

		const Nz::UInt32* opaqueVoxelMaskPtr = &opaqueBlockMask[0];
		constexpr Nz::UInt32 innerMask = 0b0111'1111'1111'1111'1111'1111'1111'1110u;
		for (unsigned int axis : { 0, 1, 2 })
		{
			for (unsigned int i = 0; i < chunkSize; ++i)
			{
				for (unsigned int j = 0; j < chunkSize; ++j)
				{
					Nz::UInt32 opaqueVoxelMask = *opaqueVoxelMaskPtr++;
					Nz::UInt32 leftFaceMask = ((~opaqueVoxelMask << 1) & opaqueVoxelMask) & innerMask;
					while (Nz::UInt32 k = Nz::FindFirstBit(leftFaceMask))
					{
						k--; // Nz::FindFirstBit returns the active bit + 1 or 0
						AddFace(GetBlockIndices(axis, i, j, k - 1), axisDir[axis * 2]);
						leftFaceMask = Nz::ClearBit(leftFaceMask, k);
					}

					Nz::UInt32 rightFaceMask = ((~opaqueVoxelMask >> 1) & opaqueVoxelMask) & innerMask;
					while (Nz::UInt32 k = Nz::FindFirstBit(rightFaceMask))
					{
						k--; // Nz::FindFirstBit returns the active bit + 1 or 0
						AddFace(GetBlockIndices(axis, i, j, k - 1), axisDir[axis * 2 + 1]);
						rightFaceMask = Nz::ClearBit(rightFaceMask, k);
					}
				}
			}
		}
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> Chunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize;
		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		return box.GetCorners();
	}

	void Chunk::DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		/* nothing to do */
	}

	void Chunk::DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		/* nothing to do */
	}

	bool Chunk::DeformPositions(Nz::SparsePtr<Nz::Vector3f> /*positions*/, std::size_t /*positionCount*/) const
	{
		/* nothing to do */
		return false;
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

		Reset();
		if (blockTypeCount > 8)
		{
			for (BlockIndex& blockIndex : m_blocks)
			{
				Nz::UInt16 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}
		else
		{
			for (BlockIndex& blockIndex : m_blocks)
			{
				Nz::UInt8 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}

		OnChunkReset();
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
		BlockIndex oldContent = m_blocks[blockIndex];
		const auto& oldBlockData = m_blockLibrary.GetBlockData(oldContent);
		assert(IsLayerRegistered(oldBlockData.layerIndex));
		m_blocks[blockIndex] = newBlock;

		if (!IsLayerRegistered(newBlockData.layerIndex))
			RegisterLayer(newBlockData.layerIndex);

		m_layers[oldBlockData.layerIndex]->collisionCellMasks[blockIndex] = false;
		m_layers[newBlockData.layerIndex]->collisionCellMasks[blockIndex] = newBlockData.hasCollisions;
		m_layers[newBlockData.layerIndex]->blockCount++;

		m_blockTypeCount[oldContent]--;
		if (newBlock >= m_blockTypeCount.size())
			m_blockTypeCount.resize(newBlock + 1);

		m_blockTypeCount[newBlock]++;

		OnBlockUpdated(this, indices, newBlock, oldBlockData.layerIndex, newBlockData.layerIndex);

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

		OnReset(this);
	}
}
