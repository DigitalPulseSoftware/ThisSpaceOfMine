// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Ship.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	Ship::Ship(float tileSize) :
	ChunkContainer(tileSize),
	m_upDirection(Nz::Vector3f::Up())
	{
	}

	FlatChunk& Ship::AddChunk(const BlockLibrary& blockLibrary, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_shared<FlatChunk>(blockLibrary, *this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

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

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this](Chunk* chunk)
		{
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, chunk->GetActiveLayerMask());
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex /*oldBlock*/, BlockIndex newBlock, std::size_t prevLayerIndex, std::size_t newLayerIndex)
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
			layerMask |= 1u << prevLayerIndex;
			if (newBlock != EmptyBlockIndex)
				layerMask |= 1u << newLayerIndex;

			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, neighborMask, layerMask);
		});

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		if (initCallback)
			chunkData.chunk->Reset(initCallback);

		FlatChunk* chunk = it->second.chunk.get();
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerAdded(this, chunk, layerIndex);

		return *it->second.chunk;
	}

	std::shared_ptr<Nz::Collider3D> Ship::BuildHullCollider() const
	{
		if (m_chunks.size() > 1)
		{
			std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;
			for (auto&& [ChunkIndices, chunkData] : m_chunks)
			{
				auto chunkCollider = chunkData.chunk->BuildCollider(0);
				if (!chunkCollider)
					continue;

				auto& childCollider = childColliders.emplace_back();
				childCollider.collider = chunkData.chunk->BuildCollider(0);
				childCollider.offset = GetChunkOffset(chunkData.chunk->GetIndices());
			}

			if (childColliders.empty())
				return nullptr;

			return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
		}
		else
		{
			if (m_chunks.empty())
				return nullptr;

			return m_chunks.begin().value().chunk->BuildCollider(0);
		}
	}

	auto Ship::ComputeGravity(const Nz::Vector3f& /*position*/) const -> GravityForce
	{
		return GravityForce {
			.direction = -m_upDirection,
			.acceleration = Constants::ShipGravityAcceleration,
			.factor = 1.f
		};
	}

	void Ship::ClearChunks()
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
		{
			Chunk* chunk = chunkData.chunk.get();
			for (std::size_t layerIndex : chunk->GetActiveLayers())
				OnChunkLayerRemove(this, chunk, layerIndex);
		}

		m_chunks.clear();
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

	void Ship::Generate(const BlockLibrary& blockLibrary, bool small)
	{
		FlatChunk& chunk = AddChunk(blockLibrary, { 0, 0, 0 });

		BlockIndex hullIndex = blockLibrary.GetBlockIndex("hull");
		if (hullIndex == InvalidBlockIndex)
			return;

		BlockIndex forcefieldIndex = blockLibrary.GetBlockIndex("forcefield");

		unsigned int boxSize = (small) ? 6 : 12;
		unsigned int height = (small) ? 4 : 6;
		Nz::Vector3ui startPos = chunk.GetSize() / 2 - Nz::Vector3ui(boxSize / 2, boxSize / 2, height / 2);

		ChunkWriteLock lock(&chunk);
		chunk.Reset();

		for (unsigned int z = 0; z < height; ++z)
		{
			for (unsigned int y = 0; y < boxSize; ++y)
			{
				for (unsigned int x = 0; x < boxSize; ++x)
				{
					if (x != 0 && x != boxSize - 1 &&
						y != 0 && y != boxSize - 1 &&
						z != 0 && z != height - 1)
						continue;

					if (x == 0 && y == boxSize / 2 && z > 0 && z < height - 1)
						chunk.UpdateBlock(startPos + Nz::Vector3ui{ x, y, z }, forcefieldIndex);
					else
						chunk.UpdateBlock(startPos + Nz::Vector3ui{ x, y, z }, hullIndex);
				}
			}
		}
	}

	void Ship::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		Chunk* chunk = it->second.chunk.get();
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerRemove(this, chunk, layerIndex);

		m_chunks.erase(it);
	}
}
