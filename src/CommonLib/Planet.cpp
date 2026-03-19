// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planet.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <Nazara/Math/Box.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <PerlinNoise.hpp>
#include <spdlog/spdlog.h>
#include <thread>

namespace tsom
{
	Planet::Planet(Nz::ApplicationBase& app, float tileSize, float cornerRadius, float gravity) :
	ChunkContainer(tileSize),
	m_app(app),
	m_cornerRadius(cornerRadius),
	m_gravity(gravity)
	{
	}

	Chunk& Planet::AddChunk(const BlockLibrary& blockLibrary, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		assert(!m_chunks.contains(indices));
		ChunkData chunkData;
		chunkData.chunk = std::make_unique<SurfaceNetsChunk>(blockLibrary, *this, indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);

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

		chunkData.onReset.Connect(chunkData.chunk->OnReset, [this, &blockLibrary](Chunk* chunk)
		{
			// Build direction blockers
			std::unique_lock chunkLock(m_chunkMutex);
			auto chunkIt = m_chunks.find(chunk->GetIndices());
			NazaraAssert(chunkIt != m_chunks.end());
			ChunkData& chunkData = chunkIt.value();
			chunkData.directionHoleCount.fill(0);

			if (chunk->HasContent())
			{
				// Left / right
				for (unsigned int z = 0; z < ChunkSize; ++z)
				{
					for (unsigned int y = 0; y < ChunkSize; ++y)
					{
						BlockIndex leftBlockIndex = chunk->GetBlockContent({ 0, y, z });
						BlockIndex rightBlockIndex = chunk->GetBlockContent({ ChunkSize - 1, y, z });

						auto& leftBlockData = blockLibrary.GetBlockData(leftBlockIndex);
						if (leftBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Left]++;

						auto& rightBlockData = blockLibrary.GetBlockData(rightBlockIndex);
						if (rightBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Right]++;
					}
				}

				// Front / back
				for (unsigned int z = 0; z < ChunkSize; ++z)
				{
					for (unsigned int x = 0; x < ChunkSize; ++x)
					{
						BlockIndex frontBlockIndex = chunk->GetBlockContent({ x, 0, z });
						BlockIndex backBlockIndex = chunk->GetBlockContent({ x, ChunkSize - 1, z });

						auto& frontBlockData = blockLibrary.GetBlockData(frontBlockIndex);
						if (frontBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Front]++;

						auto& backBlockData = blockLibrary.GetBlockData(backBlockIndex);
						if (backBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Back]++;
					}
				}

				// Down / up
				for (unsigned int y = 0; y < ChunkSize; ++y)
				{
					for (unsigned int x = 0; x < ChunkSize; ++x)
					{
						BlockIndex downBlockIndex = chunk->GetBlockContent({ x, y, 0 });
						BlockIndex upBlockIndex = chunk->GetBlockContent({ x, y, ChunkSize - 1 });

						auto& downBlockData = blockLibrary.GetBlockData(downBlockIndex);
						if (downBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Down]++;

						auto& upBlockData = blockLibrary.GetBlockData(upBlockIndex);
						if (upBlockData.isTransparent)
							chunkData.directionHoleCount[Direction::Up]++;
					}
				}

				DirectionMask oldVisibilityMask = chunkData.visibilityMask;
				chunkData.visibilityMask.Clear();

				for (auto&& [direction, holeCount] : chunkData.directionHoleCount.iter_kv())
				{
					if (holeCount > 0)
						chunkData.visibilityMask |= direction;
				}

				if (oldVisibilityMask != chunkData.visibilityMask)
					OnChunkVisibilityMaskUpdated(this, chunk, oldVisibilityMask, chunkData.visibilityMask);
			}

			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, chunk->GetActiveLayerMask());
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this, &blockLibrary](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex oldBlock, BlockIndex newBlock, std::size_t oldLayerIndex, std::size_t newLayerIndex)
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

			DirectionMask directionMask;
			if (indices.x == 0)
				directionMask |= Direction::Left;
			else if (indices.x == size.x - 1)
				directionMask |= Direction::Right;

			if (indices.y == 0)
				directionMask |= Direction::Front;
			else if (indices.y == size.y - 1)
				directionMask |= Direction::Back;

			if (indices.z == 0)
				directionMask |= Direction::Down;
			else if (indices.z == size.z - 1)
				directionMask |= Direction::Up;

			if (directionMask != 0)
			{
				auto& previousBlockData = blockLibrary.GetBlockData(oldBlock);
				auto& newBlockData = blockLibrary.GetBlockData(newBlock);

				if (previousBlockData.isTransparent != newBlockData.isTransparent)
				{
					std::unique_lock chunkLock(m_chunkMutex);

					auto chunkIt = m_chunks.find(chunk->GetIndices());
					NazaraAssert(chunkIt != m_chunks.end());
					ChunkData& chunkData = chunkIt.value();

					DirectionMask oldVisibilityMask = chunkData.visibilityMask;

					if (previousBlockData.isTransparent)
					{
						// We're putting an opaque block on a transparent one
						for (Direction direction : directionMask)
						{
							NazaraAssert(chunkData.directionHoleCount[direction] > 0);
							if (--chunkData.directionHoleCount[direction] == 0)
								chunkData.visibilityMask.Clear(direction);
						}
					}
					else
					{
						// Replacing an opaque block by a transparent one
						for (Direction direction : directionMask)
						{
							chunkData.directionHoleCount[direction]++;
							chunkData.visibilityMask |= direction;
						}
					}

					if (oldVisibilityMask != chunkData.visibilityMask)
						OnChunkVisibilityMaskUpdated(this, chunk, oldVisibilityMask, chunkData.visibilityMask);
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

		std::unique_lock chunkLock(m_chunkMutex);

		auto it = m_chunks.insert_or_assign(indices, std::move(chunkData)).first;

		chunkLock.unlock();

		if (initCallback)
			it->second.chunk->Reset(initCallback);

		Chunk* chunk = it->second.chunk.get();
		OnChunkAdded(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerAdded(this, chunk, layerIndex);

		return *it->second.chunk;
	}

	void Planet::AddChunks(const BlockLibrary& blockLibrary, const Nz::Vector3ui& chunkCount)
	{
		for (int chunkZ = 0; chunkZ < chunkCount.z; ++chunkZ)
		{
			for (int chunkY = 0; chunkY < chunkCount.y; ++chunkY)
			{
				for (int chunkX = 0; chunkX < chunkCount.x; ++chunkX)
					AddChunk(blockLibrary, { chunkX - int(chunkCount.x / 2), chunkY - int(chunkCount.y / 2), chunkZ - int(chunkCount.z / 2) });
			}
		}
	}

	auto Planet::ComputeGravity(const Nz::Vector3f& position) const -> GravityForce
	{
		constexpr float PlanetGravityCenterStartDecrease = 16.f;
		constexpr float PlanetGravityCenterNoGravity = 4.f;
		constexpr float PlanetGravitySpaceStart = 100.f;
		constexpr float PlanetGravitySpaceFinish = 150.f;
		constexpr float PlanetGravitySpaceNone = 350.f;

		// Decrease gravity near the center
		float distSq = position.SquaredDistance(GetCenter());
		if (distSq < Nz::IntegralPow(PlanetGravityCenterStartDecrease, 2))
		{
			Nz::Vector3f up = ComputeUpDirection(position);
			return GravityForce{
				.direction = -up,
				.acceleration = m_gravity,
				.factor = std::max(std::sqrt(distSq) - PlanetGravityCenterNoGravity, 0.f) / (PlanetGravityCenterStartDecrease - PlanetGravityCenterNoGravity)
			};
		}

		// Turn rounded gravity to newtonian gravity
		if (distSq > Nz::IntegralPow(PlanetGravitySpaceStart, 2))
		{
			if (distSq > Nz::IntegralPow(PlanetGravitySpaceNone, 2))
				return GravityForce::Zero();

			float dist = std::sqrt(distSq);
			float newtonianInterp;
			if (distSq > Nz::IntegralPow(PlanetGravitySpaceFinish, 2))
				newtonianInterp = 1.f;
			else
				newtonianInterp = std::max(dist - PlanetGravitySpaceStart, 0.f) / (PlanetGravitySpaceFinish - PlanetGravitySpaceStart);

			Nz::Vector3f direction = Nz::Vector3f::Normalize(GetCenter() - position);
			if (newtonianInterp < 0.99f)
				direction = Nz::Lerp(-ComputeUpDirection(position), direction, newtonianInterp);
			else
				direction = GetCenter() - position;

			direction.Normalize();

			float gravity = std::max(dist - PlanetGravitySpaceFinish, 0.f) / (PlanetGravitySpaceNone - PlanetGravitySpaceFinish);
			gravity *= gravity;

			return GravityForce{
				.direction = direction,
				.acceleration = m_gravity,
				.factor = 1.f - gravity
			};
		}

		// Regular gravity
		Nz::Vector3f up = ComputeUpDirection(position);
		return GravityForce{
			.direction = -up,
			.acceleration = m_gravity,
			.factor = 1.f
		};
	}

	Nz::Vector3f Planet::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		Nz::Vector3f center = GetCenter();

		float distToCenter = std::max({
			std::abs(position.x - center.x),
			std::abs(position.y - center.y),
			std::abs(position.z - center.z),
		});

		float innerReductionSize = std::max(distToCenter - std::max(m_cornerRadius, 1.f), 0.f);
		Nz::Boxf innerBox(center - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());

		return Nz::Vector3f::Normalize(position - innerPos);
	}

	void Planet::ClearChunks()
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
		{
			Chunk* chunk = chunkData.chunk.get();
			for (std::size_t layerIndex : chunk->GetActiveLayers())
				OnChunkLayerRemove(this, chunk, layerIndex);
		}

		m_chunks.clear();
	}

	void Planet::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Planet::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		for (auto&& [chunkIndices, chunkData] : m_chunks)
			callback(chunkIndices, *chunkData.chunk);
	}

	void Planet::GenerateChunk(Chunk& chunk, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, std::string_view scriptName)
	{
		ChunkIndices chunkIndices = chunk.GetIndices();

		bool created;
		ScriptingContext& scriptingContext = m_scriptingContexts.GetOrCreate(created, m_app);
		if (created)
		{
			scriptingContext.RegisterLibrary<MathScriptingLibrary>();
			scriptingContext.RegisterLibrary<ChunkScriptingLibrary>();
		}

		Nz::Result execResult = scriptingContext.LoadFile(fmt::format("scripts/planets/{}.lua", scriptName));
		if (!execResult)
			return;

		sol::protected_function generationFunction = execResult.GetValue();
		auto result = generationFunction(chunk, seed, chunkCount);
		if (!result.valid())
		{
			sol::error err = result;
			spdlog::error("chunk {};{};{} failed to generate: {}", chunkIndices.x, chunkIndices.y, chunkIndices.z, err.what());
		}

		std::size_t blockCount = chunk.GetBlockCount();

		sol::table blockTable = result;

		std::size_t contentSize = blockTable.size();
		if (contentSize != blockCount)
			spdlog::error("Chunk generator returned a table containing {} entries, {} expected", contentSize, blockCount);

		auto& blockLibrary = chunk.GetBlockLibrary();

		std::vector<BlockIndex> blocks(blockCount, EmptyBlockIndex);
		std::size_t maxEntries = std::min<std::size_t>(blockCount, contentSize);
		for (std::size_t i = 0; i < maxEntries; ++i)
		{
			BlockIndex blockIndex = blockTable[i + 1].get<BlockIndex>();
			if (!blockLibrary.IsValidBlock(blockIndex))
			{
				spdlog::error("Chunk:Reset content table #{} contained invalid block index \"{}\"", i, blockIndex);
				blockIndex = EmptyBlockIndex;
			}

			blocks[i] = blockIndex;
		}

		ChunkWriteLock lock(&chunk);
		chunk.Reset([&](BlockIndex* blockIndices)
		{
			std::memcpy(blockIndices, blocks.data(), blockCount * sizeof(BlockIndex));
		});
	}

	void Planet::GenerateChunks(Nz::TaskScheduler& taskScheduler, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, std::string_view scriptName)
	{
		ForEachChunk([=, this, &taskScheduler](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			if (chunk.HasContent())
				return;

			taskScheduler.AddTask([=, this, &chunk]
			{
				GenerateChunk(chunk, seed, chunkCount, scriptName);
			});
		});
	}

	void Planet::GeneratePlatform(const BlockLibrary& blockLibrary, Direction upDirection, const BlockIndices& platformCenter)
	{
		constexpr int platformSize = 15;
		constexpr unsigned int freeHeight = 10;
		const DirectionAxis& dirAxis = s_dirAxis[upDirection];

		BlockIndices coordinates = platformCenter;

		int& xPos = coordinates[dirAxis.rightAxis];
		xPos += -dirAxis.rightDir * platformSize / 2;

		int& yPos = coordinates[dirAxis.upAxis];

		int& zPos = coordinates[dirAxis.forwardAxis];
		zPos += -dirAxis.forwardDir * platformSize / 2;

		BlockIndex borderBlockIndex = blockLibrary.GetBlockIndex("copper_block");
		BlockIndex interiorBlockIndex = blockLibrary.GetBlockIndex("stone_bricks");

		BlockIndices originalCoordinates = coordinates;
		for (unsigned int y = 0; y < freeHeight; ++y)
		{
			unsigned int startingZ = zPos;
			for (unsigned int z = 0; z < platformSize; ++z)
			{
				unsigned int startingX = xPos;
				for (unsigned int x = 0; x < platformSize; ++x)
				{
					BlockIndex blockIndex;
					if (y != 0)
						blockIndex = EmptyBlockIndex;
					else if (x == 0 || x == platformSize - 1 || z == 0 || z == platformSize - 1)
						blockIndex = borderBlockIndex;
					else
						blockIndex = interiorBlockIndex;

					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = GetChunkIndicesByBlockIndices(coordinates, &innerCoordinates);
					if (Chunk* chunk = GetChunk(chunkIndices))
						chunk->UpdateBlock(innerCoordinates, blockIndex, true);

					xPos += dirAxis.rightDir;
				}

				xPos = startingX;
				zPos += dirAxis.forwardDir;
			}

			if (yPos == 0 && dirAxis.upDir < 0)
				break;

			yPos += dirAxis.upDir;
			zPos = startingZ;
		}

		// Bottom
		BlockIndex planksBlockIndex = blockLibrary.GetBlockIndex("planks");

		coordinates = originalCoordinates;
		for (unsigned int y = 1; /*no cond*/; ++y)
		{
			yPos -= dirAxis.upDir;

			bool hasEmpty = false;

			unsigned int startingZ = zPos;
			for (unsigned int z = 0; z < platformSize; ++z)
			{
				unsigned int startingX = xPos;
				for (unsigned int x = 0; x < platformSize; ++x)
				{
					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = GetChunkIndicesByBlockIndices(coordinates, &innerCoordinates);
					Chunk* chunk = GetChunk(chunkIndices);
					if (!chunk)
						continue;

					xPos += dirAxis.rightDir;

					BlockIndex blockIndex;
					if (y % 3 == 0)
					{
						if (x != 0 && x != platformSize - 1 && z != 0 && z != platformSize - 1)
							continue;
					}
					else
					{
						if (chunk->GetBlockContent(innerCoordinates) != EmptyBlockIndex)
							continue;

						if (x != 0 && x != platformSize - 1 || z != 0 && z != platformSize - 1)
							continue;
					}

					hasEmpty = true;
					chunk->UpdateBlock(innerCoordinates, planksBlockIndex, true);
				}

				xPos = startingX;
				zPos += dirAxis.forwardDir;
			}

			if (!hasEmpty)
				break;

			zPos = startingZ;
		}
	}

	void Planet::RemoveChunk(const ChunkIndices& indices)
	{
		auto it = m_chunks.find(indices);
		assert(it != m_chunks.end());

		Chunk* chunk = it->second.chunk.get();
		OnChunkRemove(this, chunk);
		for (std::size_t layerIndex : chunk->GetActiveLayers())
			OnChunkLayerRemove(this, chunk, layerIndex);

		m_chunks.erase(it);
	}
}
