// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Assert.hpp>
#include <NazaraUtils/CallOnExit.hpp>

namespace tsom
{
	inline Chunk::Chunk(ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize) :
	m_size(size),
	m_indices(indices),
	m_owner(owner),
	m_hasPerFaceCollision(false),
	m_isBatchUpdating(false),
	m_blockSize(cellSize)
	{
	}

	inline void Chunk::BeginBatchUpdate()
	{
		NazaraAssertMsg(!m_isBatchUpdating, "already in a batch update");
		m_isBatchUpdating = true;
	}

	inline void Chunk::ClearContent()
	{
		Nz::UInt32 activeLayerMask = GetActiveLayerMask();

		m_activeLayers.clear();
		m_blocks.clear();
		m_blocks.shrink_to_fit();

		for (auto& layerOpt : m_layers)
			layerOpt.reset();

		m_blockTypeCount.clear();
		m_blockTypeCount.resize(EmptyBlockIndex + 1);
		m_directionHoleCount[Direction::Left] = m_directionHoleCount[Direction::Right] = m_size.y * m_size.z;
		m_directionHoleCount[Direction::Front] = m_directionHoleCount[Direction::Back] = m_size.x * m_size.z;
		m_directionHoleCount[Direction::Down] = m_directionHoleCount[Direction::Up] = m_size.x * m_size.y;

		OnClear(this, activeLayerMask);

		if (m_visibilityMask != DirectionMask_All)
		{
			DirectionMask previousVisibilityMask = m_visibilityMask;
			m_visibilityMask = DirectionMask_All;
			OnVisibilityMaskUpdated(this, previousVisibilityMask, m_visibilityMask);
		}
	}

	inline void Chunk::ClearFlags(ChunkFlags flags)
	{
		m_flags.Clear(flags);
	}

	inline void Chunk::EndBatchUpdate()
	{
		NazaraAssertMsg(m_isBatchUpdating, "not in a batch update");
		NAZARA_DEFER({ m_isBatchUpdating = false; });

		if (!HasContent())
			return;

		if (GetBlockCount(EmptyBlockIndex) == GetBlockCount())
		{
			ClearContent();
			return;
		}

		RebuildVisibilityMask();
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
		return HasContent() ? m_blocks[blockIndex] : EmptyBlockIndex;
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
		if (!HasContent())
			return blockIndex == EmptyBlockIndex ? GetBlockCount() : 0;

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

	inline ChunkFlags Chunk::GetFlags() const
	{
		return m_flags;
	}

	inline const ChunkIndices& Chunk::GetIndices() const
	{
		return m_indices;
	}

	inline const Nz::Vector3ui& Chunk::GetSize() const
	{
		return m_size;
	}

	inline DirectionMask Chunk::GetVisibilityMask() const
	{
		return m_visibilityMask;
	}

	inline bool Chunk::HasContent() const
	{
		return !m_blocks.empty();
	}

	inline bool Chunk::HasFlags(ChunkFlags flags) const
	{
		return m_flags.Test(flags);
	}

	inline bool Chunk::HasPerFaceCollisions() const
	{
		return m_hasPerFaceCollision;
	}

	inline bool Chunk::IsLayerRegistered(std::size_t layerIndex) const
	{
		return std::find(m_activeLayers.begin(), m_activeLayers.end(), layerIndex) != m_activeLayers.end();
	}

	template<typename F>
	void Chunk::Reset(F&& func)
	{
		if (!HasContent())
			PrepareForContent();

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

	inline void Chunk::SetFlags(ChunkFlags flags)
	{
		m_flags.Set(flags);
	}

	inline bool Chunk::TryLockRead() const
	{
		return m_mutex.try_lock_shared();
	}

	inline bool Chunk::TryLockWrite()
	{
		return m_mutex.try_lock();
	}

	inline void Chunk::UnlockRead() const
	{
		m_mutex.unlock_shared();
	}

	inline void Chunk::UnlockWrite()
	{
		m_mutex.unlock();
	}

	inline void Chunk::PrepareForContent()
	{
		m_activeLayers.clear();
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
