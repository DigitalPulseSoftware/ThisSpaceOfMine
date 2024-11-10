// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
	inline Chunk::Chunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize) :
	m_size(size),
	m_indices(indices),
	m_blockLibrary(blockLibrary),
	m_owner(owner),
	m_hasPerFaceCollision(false),
	m_blockSize(cellSize)
	{
	}

	inline const Nz::Bitset<Nz::UInt64>& Chunk::GetCollisionCellMask() const
	{
		NazaraAssert(!m_blocks.empty(), "chunk has not been reset");
		return m_collisionCellMask;
	}

	inline unsigned int Chunk::GetBlockLocalIndex(const Nz::Vector3ui& indices) const
	{
		assert(indices.x < m_size.x);
		assert(indices.y < m_size.y);
		assert(indices.z < m_size.z);

		return m_size.x * (m_size.y * indices.z + indices.y) + indices.x;
	}

	inline Nz::Vector3ui Chunk::GetBlockLocalIndices(unsigned int blockIndex) const
	{
		Nz::Vector3ui indices;
		indices.x = blockIndex % m_size.x;
		indices.y = (blockIndex / m_size.x) % m_size.y;
		indices.z = blockIndex / (m_size.x * m_size.y);

		return indices;
	}

	inline BlockIndex Chunk::GetBlockContent(unsigned int blockIndex) const
	{
		NazaraAssert(!m_blocks.empty(), "chunk has not been reset");
		return m_blocks[blockIndex];
	}

	inline BlockIndex Chunk::GetBlockContent(const Nz::Vector3ui& indices) const
	{
		return GetBlockContent(GetBlockLocalIndex(indices));
	}

	inline std::size_t Chunk::GetBlockCount() const
	{
		NazaraAssert(!m_blocks.empty(), "chunk has not been reset");
		return m_blocks.size();
	}

	inline float Chunk::GetBlockSize() const
	{
		return m_blockSize;
	}

	inline ChunkContainer& Chunk::GetContainer()
	{
		return m_owner;
	}

	inline const ChunkContainer& Chunk::GetContainer() const
	{
		return m_owner;
	}

	inline const BlockIndex* Chunk::GetContent() const
	{
		NazaraAssert(!m_blocks.empty(), "chunk has not been reset");
		return m_blocks.data();
	}

	inline const ChunkIndices& Chunk::GetIndices() const
	{
		return m_indices;
	}

	inline const Nz::Vector3ui& Chunk::GetSize() const
	{
		return m_size;
	}

	inline bool Chunk::HasContent() const
	{
		return !m_blocks.empty();
	}

	inline bool Chunk::HasPerFaceCollisions() const
	{
		return m_hasPerFaceCollision;
	}

	inline void Chunk::Reset()
	{
		m_blocks.clear();
		m_blocks.resize(m_size.x * m_size.y * m_size.z, EmptyBlockIndex);

		m_collisionCellMask.Clear();
		m_collisionCellMask.Resize(m_blocks.size(), false);

		m_blockTypeCount.resize(EmptyBlockIndex + 1);
		m_blockTypeCount[EmptyBlockIndex] = m_blocks.size();
	}

	template<typename F>
	void Chunk::Reset(F&& func)
	{
		// Chunks don't have any block until they are reset
		if (!HasContent())
		{
			m_blocks.resize(m_size.x * m_size.y * m_size.z, EmptyBlockIndex);
			m_collisionCellMask.Resize(m_blocks.size(), false),
			m_blockTypeCount.resize(EmptyBlockIndex + 1);
			m_blockTypeCount[EmptyBlockIndex] = m_blocks.size();
		}

		func(m_blocks.data());
		OnChunkReset();
	}

	inline void Chunk::LockRead() const
	{
		m_mutex.lock_shared();
	}

	inline void Chunk::LockWrite()
	{
		m_mutex.lock();
	}

	inline void Chunk::UnlockRead() const
	{
		m_mutex.unlock_shared();
	}

	inline void Chunk::UnlockWrite()
	{
		m_mutex.unlock();
	}

	inline void Chunk::SetPerFaceCollision()
	{
		m_hasPerFaceCollision = true;
	}
}
