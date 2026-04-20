// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planet.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
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
			// FIXME: Nz::Signal operator() is not thread-safe!
			std::lock_guard lock(m_chunkUpdatedSignalMutex);
			OnChunkUpdated(this, chunk, NeighborChunkMask_All, chunk->GetActiveLayerMask());
		});

		chunkData.onUpdated.Connect(chunkData.chunk->OnBlockUpdated, [this, &blockLibrary](Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex /*oldBlock*/, BlockIndex newBlock, std::size_t oldLayerIndex, std::size_t newLayerIndex)
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
		constexpr float PlanetGravitySpaceStart = 200.f;
		constexpr float PlanetGravitySpaceFinish = 300.f;
		constexpr float PlanetGravitySpaceNone = 500.f;

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
		ChunkGenerator& chunkGenerator = m_chunkGenerators.GetOrCreate(created, m_app);
		if (created)
		{
			chunkGenerator.scriptingContext.RegisterLibrary<BaseScriptingLibrary>();
			chunkGenerator.scriptingContext.RegisterLibrary<MathScriptingLibrary>();
			chunkGenerator.scriptingContext.RegisterLibrary<ChunkScriptingLibrary>();

			chunkGenerator.scriptingContext.LoadDirectory("scripts/libraries");

			Nz::Result execResult = chunkGenerator.scriptingContext.LoadFile(fmt::format("scripts/planets/{}.lua", scriptName));
			if (!execResult)
				return;

			chunkGenerator.generationFunction = execResult.GetValue();
		}

		Nz::Time t1 = Nz::GetElapsedNanoseconds();
		Nz::Time t2 = Nz::GetElapsedNanoseconds();

		Nz::Time t3 = Nz::GetElapsedNanoseconds();
		auto result = chunkGenerator.generationFunction(chunk, seed, chunkCount);
		Nz::Time t4 = Nz::GetElapsedNanoseconds();

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

		Nz::Time t5 = Nz::GetElapsedNanoseconds();

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

		Nz::Time t6 = Nz::GetElapsedNanoseconds();

		ChunkWriteLock lock(&chunk);
		chunk.Reset([&](BlockIndex* blockIndices)
		{
			std::memcpy(blockIndices, blocks.data(), blockCount * sizeof(BlockIndex));
		});

		Nz::Time t7 = Nz::GetElapsedNanoseconds();

		static std::atomic_int64_t counter = 0;
		std::atomic_int64_t iterCount = ++counter;

		static std::atomic_int64_t accFile = 0;
		std::atomic_int64_t a1 = accFile.fetch_add((t2 - t1).AsMicroseconds());

		static std::atomic_int64_t accLua = 0;
		std::atomic_int64_t a2 = accLua.fetch_add((t4 - t3).AsMicroseconds());

		static std::atomic_int64_t accConvert = 0;
		std::atomic_int64_t a3 = accConvert.fetch_add((t6 - t5).AsMicroseconds());

		static std::atomic_int64_t accChunk = 0;
		std::atomic_int64_t a4 = accChunk.fetch_add((t7 - t6).AsMicroseconds());

		static std::atomic_int64_t accTotal = 0;
		std::atomic_int64_t a5 = accTotal.fetch_add((t7 - t1).AsMicroseconds());

		fmt::print("Total: {}us (load file: {}us, lua: {}us ({}), convert: {}us, chunk: {}us)\n", a5 / iterCount, a1 / iterCount, a2 / iterCount, (t4 - t3).AsMicroseconds(), a3 / iterCount, a4 / iterCount);
	}

	void Planet::GenerateChunkNative(Chunk& chunk, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, std::string_view scriptName)
	{
#if 0
		siv::PerlinNoise perlinNoise(seed);
		auto& blockLibrary = chunk.GetBlockLibrary();

		float minGrenerationFreeHeight = 0; 
		float baseFreeHeight = 30;

		float blockSize = chunk.GetBlockSize();
		float maxHeight = (chunk.GetSize() * chunkCount.x)/2 * blockSize;
		float maxGenerationHeight = maxHeight - minGrenerationFreeHeight;
		float baseHeight = maxHeight - baseFreeHeight;

		float terrainVariation1Scale = 0.06 * baseHeight;
		float terrainVariation2Scale = 0.16 * baseHeight;
		float moutainScale = 0.035 * baseHeight;
		float spikeScale = 0.2 * baseHeight;
		float caveScale = 0.06;

		std::size_t blockCount = chunk.GetBlockCount();
		std::vector<BlockIndex> blocks(blockCount, EmptyBlockIndex);

		std::size_t i = 0;
		for (std::size_t z = 0; z < ChunkSize; ++z)
		{
			for (std::size_t y = 0; y < ChunkSize; ++y)
			{
				for (std::size_t x = 0; x < ChunkSize; ++x)
				{
					BlockIndices blockPos = GetBlockIndices(chunk.GetIndices(), { x, y, z });
					Nz::Vector3f blockPosScaled(blockPos);
					blockPosScaled *= 0.5f;

					Nz::Vector3f blockPosNorm = blockPosScaled.GetNormal();
					float distToCenter = sdRoundBox(blockPosScaled, Nz::Vector3f(baseHeight), 16.0);

					//blocks[i];
				}
			}
		}
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local blockPosScaled = Vec3f(blockPos.x * 0.5, blockPos.y * 0.5, blockPos.z * 0.5)
                local blockPosNorm, distToCenter = blockPosScaled:GetNormal()
                --distToCenter = math.max(math.abs(blockPos.x * 0.5 + 0.5), math.abs(blockPos.y * 0.5 + 0.5), math.abs(blockPos.z * 0.5 + 0.5))
                distToCenter = SignedDistance.RoundBox(blockPosScaled, Vec3f(baseHeight), 16.0)

                if distToCenter > baseFreeHeight then
                    table.insert(content, emptyBlock)
                    goto continue
                end

                local blockPresence = perlin:normalizedOctave3D_01(blockPosScaled.x * caveScale, blockPosScaled.y * caveScale, blockPosScaled.z * caveScale, 4, 0.1)

                if distToCenter <= -32.0 then
                    if blockPresence >= 0.3 and blockPresence <= 0.7 then
                        if distToCenter <= -5 then
                            table.insert(content, stoneBlock)
                        else
                            table.insert(content, dirtBlock)
                        end
                    else
                        table.insert(content, stoneBlock)
                    end
                else
                    local baseMountainous = perlin:normalizedOctave3D_01((blockPosNorm.x * moutainScale)+10, blockPosNorm.y * moutainScale, blockPosNorm.z * moutainScale, 4, 0.1)
                    local mountainous
                    if baseMountainous < 0.6 then 
                        mountainous = 0
                    elseif baseMountainous < 0.8 then 
                        mountainous = 5*baseMountainous-3
                    else
                        mountainous = 1
                    end

                    local heightVariation1 = 10 * perlin:normalizedOctave3D_01(blockPosNorm.x * terrainVariation1Scale, blockPosNorm.y * terrainVariation1Scale, blockPosNorm.z * terrainVariation1Scale, 4, 0.1)
                    local heightVariation2 = 40 * mountainous * perlin:normalizedOctave3D_01((blockPosNorm.x * terrainVariation2Scale)+20, blockPosNorm.y * terrainVariation2Scale, blockPosNorm.z * terrainVariation2Scale, 4, 0.1)

                    local baseSpikeHeight = perlin:normalizedOctave3D_01((blockPosNorm.x * spikeScale)+30, blockPosNorm.y * spikeScale, blockPosNorm.z * spikeScale, 4, 0.1)
                    local spikeHeight
                    if baseSpikeHeight < 0.7 then 
                        spikeHeight = 0
                    elseif baseSpikeHeight < 0.9 then 
                        spikeHeight = 5*baseSpikeHeight-3.5
                    else
                        spikeHeight = 1
                    end
                    spikeHeight = (1-mountainous) * spikeHeight * 20

                    local height = heightVariation1 + heightVariation2 + spikeHeight

                    if distToCenter <= height then
                        if distToCenter >= height - spikeHeight then
                            table.insert(content, stoneMossyBlock)
                        elseif mountainous > 0.5 and heightVariation2 > 0.5 then
                            table.insert(content, snowBlock)
                        elseif mountainous > 0.1 then
                            table.insert(content, stoneBlock)
                        elseif baseMountainous < 0.4 then
                            table.insert(content, grassBlock)
                        else
                            table.insert(content, dirtBlock)
                        end
                    else
                        table.insert(content, emptyBlock)
                    end
                end

                ::continue::
            end
        end
    end
#endif
	}

	void Planet::GenerateChunks(Nz::TaskScheduler& taskScheduler, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, std::string_view scriptName)
	{
		m_chunkGenerators.Clear();
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
