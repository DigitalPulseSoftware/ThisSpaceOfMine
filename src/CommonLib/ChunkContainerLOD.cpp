// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkContainerLOD.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>

namespace tsom
{
	namespace NAZARA_ANONYMOUS_NAMESPACE
	{
		constexpr std::array<Nz::Vector3ui, 8> s_parentOffsets =
		{
			Nz::Vector3ui{ 0, 0, 0 },
			Nz::Vector3ui{ 0, 0, 1 },
			Nz::Vector3ui{ 0, 1, 0 },
			Nz::Vector3ui{ 0, 1, 1 },
			Nz::Vector3ui{ 1, 0, 0 },
			Nz::Vector3ui{ 1, 0, 1 },
			Nz::Vector3ui{ 1, 1, 0 },
			Nz::Vector3ui{ 1, 1, 1 }
		};
	}

	ChunkContainerLOD::ChunkContainerLOD(const ChunkContainer& parentChunkContainer) :
	ChunkContainer(parentChunkContainer.GetBlockLibrary(), parentChunkContainer.GetTileSize() * 2.0f),
	m_parentChunkContainer(parentChunkContainer)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		m_chunkOffset = parentChunkContainer.GetChunkOffset() + Nz::Vector3f(ChunkSize * parentChunkContainer.GetTileSize() * 0.5f /*+ parentChunkContainer.GetTileSize() * 0.5f*/);

		// Create LOD chunks
		parentChunkContainer.ForEachChunk([this](const ChunkIndices& /*chunkIndices*/, const Chunk& chunk)
		{
			OnParentChunkAdded(&chunk);
		});

		m_parentChunkContainer.OnChunkAdded.Connect([this](ChunkContainer* /*chunkContainer*/, Chunk* chunk)
		{
			OnParentChunkAdded(chunk);
		});

		m_parentChunkContainer.OnChunkRemove.Connect([this](ChunkContainer* /*chunkContainer*/, Chunk* chunk)
		{
			OnParentChunkRemove(chunk);
		});
	}

	Chunk& ChunkContainerLOD::AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_unique<SurfaceNetsChunk>(*this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

		chunkData.onLayerRegistered.Connect(chunkData.chunk->OnLayerRegistered, [this](Chunk* chunk, std::size_t layerIndex)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkLayerAddedSignalMutex);
			OnChunkLayerAdded(this, chunk, layerIndex);
		});

		chunkData.onLayerUnregistered.Connect(chunkData.chunk->OnLayerUnregistered, [this](Chunk* chunk, std::size_t layerIndex)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkLayerRemovedSignalMutex);
			OnChunkLayerRemove(this, chunk, layerIndex);
		});

		chunkData.onClear.Connect(chunkData.chunk->OnClear, [this](Chunk* chunk, Nz::UInt32 previousActiveLayerMask)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, previousActiveLayerMask);
		});

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this](Chunk* chunk)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, chunk->GetActiveLayerMask());
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex /*oldBlock*/, BlockIndex newBlock, std::size_t oldLayerIndex, std::size_t newLayerIndex)
		{
			NeighborChunkMask neighborMask;

			// Find every neighbor chunk required, based on block position
			std::array<Nz::Int32, 2> xs{ 0, 0 };
			std::array<Nz::Int32, 2> ys{ 0, 0 };
			std::array<Nz::Int32, 2> zs{ 0, 0 };

			std::size_t nx = 1, ny = 1, nz = 1;

			const Nz::Vector3ui& size = chunk->GetSize();

			if (indices.x == 0)
				xs[nx++] = -1;
			else if (indices.x == size.x - 1)
				xs[nx++] = 1;

			if (indices.y == 0)
				zs[nz++] = -1;
			else if (indices.y == size.y - 1)
				zs[nz++] = 1;

			if (indices.z == 0)
				ys[ny++] = -1;
			else if (indices.z == size.z - 1)
				ys[ny++] = 1;

			for (std::size_t ix = 0; ix < nx; ++ix)
			{
				for (std::size_t iy = 0; iy < ny; ++iy)
				{
					for (std::size_t iz = 0; iz < nz; ++iz)
					{
						Nz::Int32 dx = xs[ix];
						Nz::Int32 dy = ys[iy];
						Nz::Int32 dz = zs[iz];

						if (dx == 0 && dy == 0 && dz == 0)
							continue;

						ChunkIndices dir(dx, dy, dz);
						neighborMask |= ToNeighborChunk({ dx, dy, dz });
					}
				}
			}

			Nz::UInt32 layerMask = 0;
			layerMask |= 1u << oldLayerIndex;
			if (newBlock != EmptyBlockIndex)
				layerMask |= 1u << newLayerIndex;

			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, neighborMask, layerMask);
		});

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		if (initCallback)
			it->second.chunk->Reset(initCallback);

		Chunk* chunk = it->second.chunk.get();
		OnChunkAdded(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerAdded(this, chunk, layerIndex);

		return *it->second.chunk;
	}

	void ChunkContainerLOD::ClearChunks()
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
		{
			Chunk* chunk = chunkData.chunk.get();
			OnChunkRemove(this, chunk);
			for (std::size_t layerIndex : chunk->GetActiveLayers())
				OnChunkLayerRemove(this, chunk, layerIndex);
		}

		m_chunks.clear();
	}

	void ChunkContainerLOD::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void ChunkContainerLOD::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void ChunkContainerLOD::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		Chunk* chunk = it->second.chunk.get();
		OnChunkRemove(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerRemove(this, chunk, layerIndex);

		m_chunks.erase(it);
		m_invalidatedChunks.erase(indices);
	}

	void ChunkContainerLOD::Update()
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		for (auto&& [chunkIndices, invalidatedBlocks] : m_invalidatedChunks)
		{
			Chunk* chunk = GetChunk(chunkIndices);
			NazaraAssert(chunk);

			/*if (!invalidatedBlocks.empty())
			{
				ChunkIndices parentIndices = ToParentChunkIndices(chunkIndices);

				MultiChunkReadLock<s_parentOffsets.size()> readLock;
				NeighborChunkArray neighborChunkArray;

				for (const Nz::Vector3ui& offset : s_parentOffsets)
				{
					const Chunk* chunk = m_parentChunkContainer.GetChunk(parentIndices + ChunkIndices(offset));
					neighborChunkArray[ToNeighborChunkIndices(offset)] = chunk;

					if (chunk)
						readLock.AddChunk(chunk);
				}
				readLock.Lock();

				for (const Nz::Vector3ui& indices : invalidatedBlocks)
				{

				}
			}
			else*/
				RebuildChunkLOD(*chunk);
		}
		m_invalidatedChunks.clear();
	}

	BlockIndex ChunkContainerLOD::ComputeBlockIndex(const NeighborChunkArray& neighborChunkArray, const Chunk& chunkLOD, const Nz::Vector3ui& blockIndicesLOD)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		ChunkIndices parentIndices = ToParentChunkIndices(chunkLOD.GetIndices());

		std::array<BlockIndex, 8> parentContent;

		BlockIndex* parentContentPtr = parentContent.data();
		for (const Nz::Vector3ui& indices : s_parentOffsets)
		{
			Nz::Vector3ui parentBlockIndices = blockIndicesLOD * 2 + indices;

			Nz::Vector3ui targetChunkOffset = Nz::Vector3ui(parentBlockIndices.x, parentBlockIndices.z, parentBlockIndices.y) / ChunkSize;
			parentBlockIndices %= ChunkSize;

			const Chunk* parentChunk = neighborChunkArray[ToNeighborChunkIndices(targetChunkOffset)];
			if (parentChunk && parentChunk->HasContent())
				*parentContentPtr++ = parentChunk->GetBlockContent(parentBlockIndices);
			else
				*parentContentPtr++ = EmptyBlockIndex;
		}

		// Find the most represented block
		// https://www.cs.utexas.edu/~moore/best-ideas/mjrty/index.html
		BlockIndex largestBlockType = EmptyBlockIndex;
		std::size_t largestBlockCount = 0;

		for (BlockIndex blockType : parentContent)
		{
			if (largestBlockType == blockType)
				largestBlockCount++;
			else if (largestBlockCount > 0)
				largestBlockCount--;
			else
				largestBlockType = blockType;
		}

		return largestBlockType;
	}

	void ChunkContainerLOD::OnParentChunkAdded(const Chunk* parentChunk)
	{
		assert(parentChunk->GetSize() == Nz::Vector3ui(ChunkSize));

		ChunkIndices parentIndices = parentChunk->GetIndices();

		if (auto parentIt = m_parentChunks.find(parentIndices); parentIt == m_parentChunks.end())
		{
			ParentChunkData& parentChunkData = m_parentChunks[parentIndices];
			parentChunkData.onClear.Connect(parentChunk->OnClear, this, &ChunkContainerLOD::OnParentChunkCleared);
			parentChunkData.onReset.Connect(parentChunk->OnReset, this, &ChunkContainerLOD::OnParentChunkReset);
			parentChunkData.onUpdated.Connect(parentChunk->OnBlockUpdated, this, &ChunkContainerLOD::OnParentChunkBlockUpdated);
		}

		ChunkIndices lodIndices = ToLODChunkIndices(parentIndices);

		Chunk* chunkLOD;
		if (auto it = m_chunks.find(lodIndices); it == m_chunks.end())
			chunkLOD = &AddChunk(lodIndices);
		else
			chunkLOD = it->second.chunk.get();

		m_invalidatedChunks.insert_or_assign(lodIndices, ChunkInvalidationData{});
	}

	void ChunkContainerLOD::OnParentChunkBlockUpdated(Chunk* parentChunk, const Nz::Vector3ui& indices, BlockIndex /*oldBlock*/, BlockIndex /*newBlock*/, std::size_t /*prevLayerIndex*/, std::size_t /*newLayerIndex*/)
	{
		ChunkIndices lodIndices = ToLODChunkIndices(parentChunk->GetIndices());

		// Only insert partial update if a full update is not yet planned
		auto it = m_invalidatedChunks.find(lodIndices);
		if (it == m_invalidatedChunks.end() || it.value().size() + 1 >= FullRebuildThreshold) //< handled full rebuild threshold
			m_invalidatedChunks.emplace(lodIndices, ChunkInvalidationData{ indices });
		else
			it.value().push_back(indices);
	}

	void ChunkContainerLOD::OnParentChunkCleared(Chunk* parentChunk, Nz::UInt32 /*previousActiveLayerMask*/)
	{
		m_invalidatedChunks.insert_or_assign(ToLODChunkIndices(parentChunk->GetIndices()), ChunkInvalidationData{});
	}

	void ChunkContainerLOD::OnParentChunkRemove(Chunk* parentChunk)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		ChunkIndices chunkIndices = parentChunk->GetIndices();
		m_parentChunks.erase(chunkIndices);

		// Check if any of the 8 parent chunks is still present, if not we can remove the lod chunk
		ChunkIndices lodIndices = ToLODChunkIndices(chunkIndices);
		ChunkIndices parentChunkIndices = ToParentChunkIndices(lodIndices);
		for (const Nz::Vector3ui& indices : s_parentOffsets)
		{
			if (m_parentChunks.contains(parentChunkIndices + ChunkIndices(indices)))
				return;
		}

		RemoveChunk(lodIndices);
	}

	void ChunkContainerLOD::OnParentChunkReset(Chunk* parentChunk)
	{
		m_invalidatedChunks.insert_or_assign(ToLODChunkIndices(parentChunk->GetIndices()), ChunkInvalidationData{});
	}

	void ChunkContainerLOD::RebuildChunkLOD(Chunk& chunkLOD)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		ChunkWriteLock writeLock(&chunkLOD);
		chunkLOD.Reset([&](BlockIndex* blockIndices)
		{
			ChunkIndices parentIndices = ToParentChunkIndices(chunkLOD.GetIndices());

			MultiChunkReadLock<s_parentOffsets.size()> readLock;
			NeighborChunkArray neighborChunkArray;

			for (const Nz::Vector3ui& offset : s_parentOffsets)
			{
				const Chunk* chunk = m_parentChunkContainer.GetChunk(parentIndices + ChunkIndices(offset));
				neighborChunkArray[ToNeighborChunkIndices(offset)] = chunk;

				if (chunk)
					readLock.AddChunk(chunk);
			}
			readLock.Lock();

			Nz::Vector3ui chunkSize = chunkLOD.GetSize();
			for (unsigned int z = 0; z < chunkSize.z; ++z)
			{
				for (unsigned int y = 0; y < chunkSize.y; ++y)
				{
					for (unsigned int x = 0; x < chunkSize.x; ++x)
					{
						*blockIndices++ = ComputeBlockIndex(neighborChunkArray, chunkLOD, { x, y, z });
					}
				}
			}
		});
	}
}
