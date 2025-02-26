// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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

	inline std::span<const std::size_t> Chunk::GetActiveLayers() const
	{
		return { m_activeLayers.data(), m_activeLayers.size() };
	}

	inline Nz::UInt32 Chunk::GetActiveLayerMask() const
	{
		Nz::UInt32 layerMask = 0u;
		for (std::size_t layerIndex : m_activeLayers)
			layerMask |= Nz::UInt32(1u << layerIndex);

		return layerMask;
	}

	inline const Nz::Bitset<Nz::UInt64>& Chunk::GetCollisionCellMask(std::size_t layerIndex) const
	{
		NazaraAssertMsg(!m_blocks.empty(), "chunk has not been reset");
		NazaraAssertMsg(m_layers[layerIndex].has_value(), "layer %zu is not active", layerIndex);
		return m_layers[layerIndex]->collisionCellMasks;
	}

	inline const BlockLibrary& Chunk::GetBlockLibrary() const
	{
		return m_blockLibrary;
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
		NazaraAssertMsg(!m_blocks.empty(), "chunk has not been reset");
		return m_blocks[blockIndex];
	}

	inline BlockIndex Chunk::GetBlockContent(const Nz::Vector3ui& indices) const
	{
		return GetBlockContent(GetBlockLocalIndex(indices));
	}

	inline std::size_t Chunk::GetBlockCount() const
	{
		return m_size.x * m_size.y * m_size.z;
	}

	inline Nz::UInt16 Chunk::GetBlockCount(std::size_t blockIndex) const
	{
		if (blockIndex >= m_blockTypeCount.size())
			return 0;

		return m_blockTypeCount[blockIndex];
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
		NazaraAssertMsg(!m_blocks.empty(), "chunk has not been reset");
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

	inline bool Chunk::IsLayerRegistered(std::size_t layerIndex) const
	{
		return std::find(m_activeLayers.begin(), m_activeLayers.end(), layerIndex) != m_activeLayers.end();
	}

	inline void Chunk::Reset()
	{
		m_blocks.clear();
		m_blocks.resize(m_size.x * m_size.y * m_size.z, EmptyBlockIndex);

		for (auto& layerOpt : m_layers)
			layerOpt.reset();

		// Create first layer (for empty block)
		RegisterLayer(0);

		m_blockTypeCount.resize(EmptyBlockIndex + 1);
		m_blockTypeCount[EmptyBlockIndex] = m_blocks.size();
		m_layers[0]->blockCount = m_blocks.size();
	}

	template<typename F>
	void Chunk::Reset(F&& func)
	{
		// Chunks don't have any block until they are reset
		if (!HasContent())
		{
			m_blocks.resize(m_size.x * m_size.y * m_size.z, EmptyBlockIndex);
			m_blockTypeCount.resize(EmptyBlockIndex + 1);
			m_blockTypeCount[EmptyBlockIndex] = m_blocks.size();

			for (auto& layerOpt : m_layers)
				layerOpt.reset();

			// Create first layer (empty block)
			RegisterLayer(0);
			m_layers[0]->blockCount = m_blocks.size();
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

	inline void Chunk::RegisterLayer(std::size_t layerIndex)
	{
		NazaraAssertMsg(!IsLayerRegistered(layerIndex), "layer %zu is already registered", layerIndex);

		auto& layer = m_layers[layerIndex].emplace();
		layer.collisionCellMasks.Resize(m_blocks.size(), false);
		m_activeLayers.push_back(layerIndex);
		std::sort(m_activeLayers.begin(), m_activeLayers.end());

		OnLayerRegistered(this, layerIndex);
	}

	inline void Chunk::SetPerFaceCollision()
	{
		m_hasPerFaceCollision = true;
	}

	inline void Chunk::UnregisterLayer(std::size_t layerIndex)
	{
		auto it = std::find(m_activeLayers.begin(), m_activeLayers.end(), layerIndex);
		NazaraAssertMsg(it != m_activeLayers.end(), "layer %zu is not registered", layerIndex);

		m_layers[layerIndex].reset();
		m_activeLayers.erase(it);
		// Already sorted

		OnLayerUnregistered(this, layerIndex);
	}
}
