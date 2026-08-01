// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <CommonLib/Systems/GravityPhysicsSystem.hpp>
#include <CommonLib/Systems/PlanetSystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/DatabaseComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <ServerLib/Systems/PlanetDatabaseSystem.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/ByteArray.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Hash/SHA256.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <nlohmann/json.hpp>

template<>
struct fmt::formatter<tsom::Direction> : formatter<string_view>
{
	// parse is inherited from formatter<string_view>.

	auto format(tsom::Direction dir, format_context& ctx) const -> format_context::iterator
	{
		string_view name = "unknown";
		switch (dir) {
			case tsom::Direction::Back: name = "Back"; break;
			case tsom::Direction::Down: name = "Down"; break;
			case tsom::Direction::Front: name = "Front"; break;
			case tsom::Direction::Left: name = "Left"; break;
			case tsom::Direction::Right: name = "Right"; break;
			case tsom::Direction::Up: name = "Up"; break;
		}
		return formatter<string_view>::format(name, ctx);
	}
};

template<typename T>
struct fmt::formatter<Nz::Flags<T>> : formatter<string_view>
{
	// parse is inherited from formatter<string_view>.

	auto format(Nz::Flags<T> flags, format_context& ctx) const -> format_context::iterator
	{
		std::string flagStr;
		for (T flag : flags)
		{
			if (!flagStr.empty())
				flagStr += " | ";

			flagStr += fmt::format("{}", flag);
		}

		return formatter<string_view>::format(flagStr, ctx);
	}
};

namespace tsom
{
	constexpr unsigned int s_chunkVersion = 1;

	ServerPlanetEnvironment::ServerPlanetEnvironment(ServerInstance& serverInstance, std::optional<Nz::UInt32> databaseId, std::string generatorName, const Nz::Vector3ui& chunkCount, std::string_view planetType, std::vector<EntityProperty>&& properties) :
	ServerEnvironment(serverInstance, ServerEnvironmentType::Planet, true),
	m_databaseId(databaseId),
	m_generatorName(std::move(generatorName)),
	m_chunkCount(chunkCount)
	{
		// Compute generator hash
		auto& filesystem = serverInstance.GetApplication().GetComponent<Nz::FilesystemAppComponent>();
		if (std::shared_ptr<Nz::Stream> generatorStream = filesystem.GetFile(fmt::format("scripts/planets/{}.lua", m_generatorName)))
		{
			Nz::SHA256Hasher hash;
			hash.Begin();
			Nz::HashAppend(hash, *generatorStream);
			m_generatorHash = hash.End().ToHex();
		}

		m_world->GetRegistry().ctx().emplace<ServerPlanetEnvironment*>(this);
		m_world->AddSystem<EnvironmentSwitchSystem>();

		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_planetEntity = CreateEntity();
		m_planetEntity.emplace<Nz::NodeComponent>();
		m_planetEntity.emplace<NetworkedComponent>();

		std::shared_ptr<const EntityClass> planetClass = serverInstance.GetEntityRegistry().FindClass(planetType);
		NazaraAssert(planetClass);

		// Build property map for Chunk generation
		for (Nz::UInt32 propertyIndex = 0; propertyIndex < properties.size(); ++propertyIndex)
		{
			const auto& property = planetClass->GetProperty(propertyIndex);
			m_planetProperties.emplace(property.name, properties[propertyIndex]);
		}

		auto& entityInstance = m_planetEntity.emplace<ClassInstanceComponent>(planetClass, std::move(properties));
		planetClass->InitAndActivateEntity(m_planetEntity);

		auto& planetComponent = m_planetEntity.get<PlanetComponent>();

		m_chunkLoadingData = std::make_shared<ChunkLoadingData>();
		m_chunkLoadingData->chunkCount = m_chunkCount;
		m_chunkLoadingData->planet = planetComponent.planet;

		for (int z = 0; z < m_chunkCount.z; ++z)
		{
			for (int x = 0; x < m_chunkCount.x; ++x)
			{
				ChunkIndices chunkIndices = ChunkIndices(-int(m_chunkCount.x / 2) + x, -int(m_chunkCount.y / 2) + m_chunkCount.y - 1, -int(m_chunkCount.z / 2) + z);
				m_chunkLoadingData->remainingChunks[chunkIndices] |= (x % 2) == (z % 2) ? Direction::Up : Direction::Down;
				m_chunkLoadingData->visitedChunks.emplace(chunkIndices, true);
			}
		}

		// Since the planet will not live longer than the environment it's okay to directly bind them instead of using slots
		GetPlanet().OnChunkAdded.Connect([chunkLoadingData = m_chunkLoadingData](ChunkContainer* /*planet*/, Chunk* chunk)
		{
			chunk->OnVisibilityMaskUpdated.Connect([chunkLoadingData](Chunk* chunk, DirectionMask oldVisibilityMask, DirectionMask newVisibilityMask)
			{
				DirectionMask newDirectionMask = newVisibilityMask & ~oldVisibilityMask;

				std::unique_lock lock(chunkLoadingData->mutex);
				chunkLoadingData->visitedChunks[chunk->GetIndices()] = true;
				chunkLoadingData->HandleChunkLoaded(chunk->GetIndices(), newDirectionMask);
			});
		});

		// We want the player to be able to breathe 5s per empty block count
		// the player breathe 100ml per second
		Nz::UInt64 oxygenAmount = Constants::SecondsToEmptyOxygenBlock * Nz::UInt64(Constants::PlayerOxygenConsumption) * m_chunkCount.x * m_chunkCount.y * m_chunkCount.z;
		m_atmosphere.SetGasAmount(GasType::Oxygen, oxygenAmount);

		// We also want oxygen to be 21% of the atmosphere and have the rest as nitrogen
		m_atmosphere.SetGasAmount(GasType::Nitrogen, oxygenAmount * (100 - Constants::OxygenAtmospherePct) / Constants::OxygenAtmospherePct);

		planetComponent.planet->OnChunkUpdated.Connect([this](ChunkContainer* /*planet*/, Chunk* chunk, NeighborChunkMask /*neighborMask*/, Nz::UInt32 /*layerMask*/)
		{
			if (chunk->HasFlags(ChunkFlag::SaveToDatabase))
				m_dirtyChunks.insert(chunk->GetIndices());
		});

		auto& physicsSystem = m_world->GetSystem<Nz::Physics3DSystem>();
		m_world->AddSystem<BuoyancySystem>(*planetComponent.planet, physicsSystem.GetPhysWorld(), m_debugDrawer.get());
		m_world->AddSystem<GravityPhysicsSystem>(*planetComponent.planet, physicsSystem.GetPhysWorld());
		m_world->AddSystem<PlanetSystem>();

		if (m_databaseId)
		{
			ServerDatabase& serverDatabase = m_serverInstance.GetThreadServerDatabase();
			m_world->AddSystem<PlanetDatabaseSystem>(serverDatabase, *m_databaseId);

			LoadEntitiesFromDatabase();
		}
	}

	ServerPlanetEnvironment::~ServerPlanetEnvironment()
	{
		ClearEntities();

		m_world->GetRegistry().ctx().erase<ServerPlanetEnvironment*>();
	}

	Nz::Boxf ServerPlanetEnvironment::ComputeBoundingBox() const
	{
		float halfChunkSize = Planet::ChunkSize * 0.5f * GetPlanet().GetTileSize();
		return Nz::Boxf::FromExtents(Nz::Vector3f(m_chunkCount) * -halfChunkSize, Nz::Vector3f(m_chunkCount) * halfChunkSize);
	}

	entt::handle ServerPlanetEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	void ServerPlanetEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback)
	{
		ServerEnvironment::ForEachAtmosphere(callback);

		callback(&m_atmosphere);
	}

	void ServerPlanetEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const
	{
		ServerEnvironment::ForEachAtmosphere(callback);

		callback(&m_atmosphere);
	}

	const GravityController* ServerPlanetEnvironment::GetGravityController() const
	{
		return m_planetEntity.get<PlanetComponent>().planet.get();
	}

	Planet& ServerPlanetEnvironment::GetPlanet()
	{
		return *m_planetEntity.get<PlanetComponent>().planet;
	}

	const Planet& ServerPlanetEnvironment::GetPlanet() const
	{
		return *m_planetEntity.get<PlanetComponent>().planet;
	}

	void ServerPlanetEnvironment::OnSave()
	{
		if (!m_databaseId)
			return;

		if (!m_dirtyChunks.empty())
		{
			spdlog::info("saving {} dirty chunks...", m_dirtyChunks.size());

			BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();
			ServerDatabase& serverDatabase = m_serverInstance.GetThreadServerDatabase();
			Planet& planet = GetPlanet();

			serverDatabase.Transaction([&](ServerDatabase& database)
			{
				Nz::ByteArray byteArray;
				for (const ChunkIndices& chunkIndices : m_dirtyChunks)
				{
					Chunk* chunk = planet.GetChunk(chunkIndices);

					ChunkWriteLock lock(chunk); //< write lock because we clear its flags

					byteArray.Clear();

					Nz::ByteStream byteStream(&byteArray);
					chunk->Serialize(byteStream);

					Nz::UInt32 decompressedSize = Nz::SafeCaster(byteArray.GetSize());

					std::optional compressedDataOpt = binaryCompressor.Compress(byteArray.GetBuffer(), byteArray.GetSize());
					if NAZARA_UNLIKELY(!compressedDataOpt)
						throw std::runtime_error("chunk compression failed");

					std::span<Nz::UInt8>& compressedData = *compressedDataOpt;

					// Reuse byteArray
					byteArray.Clear();
					byteArray.Resize(sizeof(Nz::UInt32) + compressedData.size());

					decompressedSize = Nz::HostToLittleEndian(decompressedSize);
					std::memcpy(&byteArray[0], &decompressedSize, sizeof(decompressedSize));
					std::memcpy(&byteArray[sizeof(decompressedSize)], &compressedData[0], compressedData.size());

					serverDatabase.StorePlanetChunk(Database::PlanetChunk{
						.planetId = *m_databaseId,
						.position = chunkIndices,
						.version = s_chunkVersion,
						.chunkData = std::span(byteArray.GetBuffer(), byteArray.GetSize()),
						.cacheState = !chunk->HasFlags(ChunkFlag::PlayerModified)
					});

					chunk->ClearFlags(ChunkFlag::SaveToDatabase);
				}

				return true;
			});

			m_dirtyChunks.clear();
		}

		if (PlanetDatabaseSystem* databaseSystem = m_world->TryGetSystem<PlanetDatabaseSystem>())
			databaseSystem->Save();
	}

	void ServerPlanetEnvironment::Update()
	{
		std::unique_lock lock(m_chunkLoadingData->mutex);

		for (auto&& [indices, originDir] : m_chunkLoadingData->remainingChunks)
		{
			spdlog::debug("loading chunk {};{};{} from {} ({} remaining)", indices.x, indices.y, indices.z, originDir, m_chunkLoadingData->remainingChunks.size());

			Chunk* chunk = GetPlanet().GetChunk(indices);
			if (!chunk)
				chunk = &GetPlanet().AddChunk(m_serverInstance.GetBlockLibrary(), indices);

			auto& taskScheduler = m_serverInstance.GetApplication().GetComponent<Nz::TaskSchedulerAppComponent>();
			bool enableChunkCache = m_serverInstance.GetConfig().enableChunkCache;

			m_chunkLoadingData->chunkLoadingCount++;
			taskScheduler.AddTask([chunk, enableChunkCache, serverInstance = &m_serverInstance, databaseId = m_databaseId, chunkLoadingData = m_chunkLoadingData, chunkCount = m_chunkCount, generatorName = m_generatorName, generatorHash = m_generatorHash, originDir = originDir, properties = m_planetProperties]
			{
				ServerDatabase& serverDatabase = serverInstance->GetThreadServerDatabase();
				BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();

				ChunkIndices chunkIndices = chunk->GetIndices();

				bool chunkFound = false;
				if (databaseId)
				{
					chunkFound = serverDatabase.GetPlanetChunk(*databaseId, chunkIndices, [&](Database::PlanetChunk&& planetChunk)
					{
						if (planetChunk.version != s_chunkVersion)
							throw std::runtime_error(fmt::format("unhandled version {}", planetChunk.version));

						// Chunk data has decompressedSize first
						Nz::UInt32 decompressedSize;
						std::memcpy(&decompressedSize, &planetChunk.chunkData[0], sizeof(decompressedSize));
						decompressedSize = Nz::LittleEndianToHost(decompressedSize);

						std::vector<Nz::UInt8> decompressedData(decompressedSize);
						std::optional compressedDataOpt = binaryCompressor.Decompress(planetChunk.chunkData.data() + sizeof(decompressedSize), planetChunk.chunkData.size() - sizeof(decompressedSize), decompressedData.data(), decompressedData.size());
						if (!compressedDataOpt)
							throw std::runtime_error("chunk decompression failed");

						if (*compressedDataOpt != decompressedSize)
							throw std::runtime_error("chunk decompression failed (corrupt size)");

						Nz::ByteStream byteStream(decompressedData.data(), decompressedData.size());

						ChunkWriteLock lock(chunk);
						chunk->Deserialize(byteStream);
						if (!planetChunk.cacheState)
							chunk->SetFlags(ChunkFlag::PlayerModified);
					});
				}

				if (!chunkFound)
					chunkLoadingData->planet->GenerateChunk(*chunk, chunkCount, generatorName, properties);

				const auto& faceVisibilityMasks = chunk->GetFaceVisibilityMasks();

				DirectionMask visibilityMask;
				for (Direction dir : originDir)
					visibilityMask |= faceVisibilityMasks[s_oppositeDirections[dir]];

				std::unique_lock lock(chunkLoadingData->mutex);
				chunkLoadingData->HandleChunkLoaded(chunk->GetIndices(), visibilityMask);
				if (!chunkFound && enableChunkCache)
					chunkLoadingData->generatedChunks.insert(chunk->GetIndices());

				if (--chunkLoadingData->chunkLoadingCount == 0 && chunkLoadingData->remainingChunks.empty())
					spdlog::debug("planet chunk loading finished, total chunks: {} (out of {})", chunkLoadingData->visitedChunks.size(), chunkCount.x * chunkCount.y * chunkCount.z);
			});
		}
		m_chunkLoadingData->remainingChunks.clear();

		if (!m_dirtyChunks.empty())
		{
			for (const ChunkIndices& chunkIndices : m_chunkLoadingData->generatedChunks)
				m_dirtyChunks.emplace(chunkIndices);
		}
		else
			m_dirtyChunks = std::move(m_chunkLoadingData->generatedChunks);
		
		m_chunkLoadingData->generatedChunks.clear();
	}

	ServerAtmosphere* ServerPlanetEnvironment::GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position)
	{
		Planet& planet = GetPlanet();
		if (position.SquaredDistance(planet.GetCenter()) > Nz::IntegralPow(150, 2))
			return nullptr; //< too far away

		return &m_atmosphere;
	}

	void ServerPlanetEnvironment::LoadChunksFromDatabase()
	{
		NazaraAssert(m_databaseId);

		ServerDatabase& serverDatabase = m_serverInstance.GetThreadServerDatabase();

		BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();
		auto& blockLibrary = m_serverInstance.GetBlockLibrary();
		Planet& planet = GetPlanet();

		serverDatabase.GetAllPlanetChunks(*m_databaseId, [&](Database::PlanetChunk&& planetChunk)
		{
			if (planetChunk.version != s_chunkVersion)
				throw std::runtime_error(fmt::format("unhandled version {}", planetChunk.version));

			// Chunk data has decompressedSize first
			Nz::UInt32 decompressedSize;
			std::memcpy(&decompressedSize, &planetChunk.chunkData[0], sizeof(decompressedSize));
			decompressedSize = Nz::LittleEndianToHost(decompressedSize);

			std::vector<Nz::UInt8> decompressedData(decompressedSize);
			std::optional compressedDataOpt = binaryCompressor.Decompress(planetChunk.chunkData.data() + sizeof(decompressedSize), planetChunk.chunkData.size() - sizeof(decompressedSize), decompressedData.data(), decompressedData.size());
			if (!compressedDataOpt)
				throw std::runtime_error("chunk decompression failed");

			if (*compressedDataOpt != decompressedSize)
				throw std::runtime_error("chunk decompression failed (corrupt size)");

			Nz::ByteStream byteStream(decompressedData.data(), decompressedData.size());
			Chunk* chunk = planet.GetChunk(planetChunk.position);
			if (!chunk)
				chunk = &planet.AddChunk(blockLibrary, planetChunk.position);

			ChunkWriteLock lock(chunk);
			chunk->Deserialize(byteStream);

			return true;
		});
	}

	void ServerPlanetEnvironment::LoadEntitiesFromDatabase()
	{
		ServerDatabase& serverDatabase = m_serverInstance.GetThreadServerDatabase();
		serverDatabase.GetAllPlanetEntities(*m_databaseId, [&](Database::PlanetEntity&& planetEntity)
		{
			std::shared_ptr<const EntityClass> entityClass = m_serverInstance.GetEntityRegistry().FindClass(planetEntity.className);
			if (!entityClass)
			{
				NazaraError("Database entity {} has unknown class {}", planetEntity.id, planetEntity.className);
				return true;
			}

			entt::handle entity = CreateEntity();
			entity.emplace<Nz::NodeComponent>(planetEntity.position, planetEntity.rotation);
			entity.emplace<NetworkedComponent>();
			entity.emplace<DatabaseComponent>(planetEntity.uniqueId, planetEntity.id);

			entity.emplace<ClassInstanceComponent>(entityClass, entityClass->PropertiesFromJson(planetEntity.properties));
			entityClass->InitAndActivateEntity(entity);

			return true;
		});
	}

	void ServerPlanetEnvironment::ChunkLoadingData::HandleChunkLoaded(const ChunkIndices& chunkIndices, DirectionMask visibilityMask)
	{
		ChunkIndices minIndices(-int(chunkCount.x / 2), -int(chunkCount.y / 2), -int(chunkCount.z / 2));
		ChunkIndices maxIndices = minIndices + ChunkIndices(chunkCount) - ChunkIndices(1);

		// Direct neighbor can trigger indirect neighbor
		bool isPrimaryChunk = Nz::Retrieve(visitedChunks, chunkIndices);

		for (Direction direction : DirectionMask_All)
		{
			bool isNeighborPrimaryChunk = visibilityMask.Test(direction);
			if (!isPrimaryChunk && !isNeighborPrimaryChunk)
				continue;

			ChunkIndices neighborIndices = chunkIndices + s_chunkDirOffset[direction];
			if (neighborIndices.x < minIndices.x || neighborIndices.x > maxIndices.x
			 || neighborIndices.y < minIndices.y || neighborIndices.y > maxIndices.y
			 || neighborIndices.z < minIndices.z || neighborIndices.z > maxIndices.z)
				continue;

			auto it = visitedChunks.find(neighborIndices);
			if (it != visitedChunks.end())
			{
				it.value() |= isNeighborPrimaryChunk;
				continue;
			}

			visitedChunks.emplace(neighborIndices, isNeighborPrimaryChunk);
			remainingChunks[neighborIndices] |= direction;
		}
	}
}
