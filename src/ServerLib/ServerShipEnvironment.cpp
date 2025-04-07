// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerShipEnvironment.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <CommonLib/Systems/GravityPhysicsSystem.hpp>
#include <CommonLib/Systems/ShipSystem.hpp>
#include <CommonLib/Utility/JsonSerialization.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/AtmosphereCarrier.hpp>
#include <ServerLib/Components/DatabaseComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <ServerLib/Components/ShipExteriorComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fmt/color.h>
#include <nlohmann/json.hpp>

namespace tsom
{
	namespace
	{
		constexpr unsigned int ShipChunkBlockCount = Ship::ChunkSize * Ship::ChunkSize * Ship::ChunkSize;
	}

	ServerShipEnvironment::ServerShipEnvironment(ServerInstance& serverInstance, const std::optional<Nz::Uuid>& playerUuid, int saveSlot) :
	ServerEnvironment(serverInstance, ServerEnvironmentType::Ship, false),
	m_playerUuid(playerUuid),
	m_shouldSave(std::make_shared<bool>(false)),
	m_exteriorEnvironment(nullptr),
	m_isInteriorAreaColliderGenerated(false),
	m_isInteriorAreaColliderInvalidated(false),
	m_saveSlot(saveSlot)
	{
		auto& app = serverInstance.GetApplication();
		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_world->GetRegistry().ctx().emplace<ServerShipEnvironment*>(this);

		m_shipEntity = CreateEntity();
		m_shipEntity.emplace<Nz::NodeComponent>();
		m_shipEntity.emplace<NetworkedComponent>();

		std::shared_ptr<const EntityClass> shipClass = serverInstance.GetEntityRegistry().FindClass("ship");
		NazaraAssert(shipClass);

		auto& entityInstance = m_shipEntity.emplace<ClassInstanceComponent>(shipClass);
		entityInstance.UpdateProperty<EntityPropertyType::Float>("CellSize", 1.f);

		shipClass->InitAndActivateEntity(m_shipEntity);

		auto& shipComponent = m_shipEntity.get<ShipComponent>();
		shipComponent.ship->OnChunkLayerAdded.Connect([this](ChunkContainer*, Chunk* chunk, std::size_t /*layerIndex*/)
		{
			auto& chunkData = m_chunkData[chunk->GetIndices()];
			chunkData.blockSize = chunk->GetBlockSize();
			m_invalidatedChunks.emplace(chunk);
		});

		shipComponent.ship->OnChunkLayerRemove.Connect([this](ChunkContainer*, Chunk* chunk, std::size_t /*layerIndex*/)
		{
			const ChunkIndices& indices = chunk->GetIndices();
			m_chunkData.erase(indices);
			m_invalidatedChunks.erase(chunk);

			if (auto it = m_areaUpdateJobs.find(indices); it != m_areaUpdateJobs.end())
			{
				it->second->isCancelled = true;
				m_areaUpdateJobs.erase(indices);
			}

			if (auto it = m_triggerUpdateJobs.find(indices); it != m_triggerUpdateJobs.end())
			{
				it->second->isCancelled = true;
				m_triggerUpdateJobs.erase(indices);
			}
		});

		shipComponent.ship->OnChunkUpdated.Connect([this](ChunkContainer*, Chunk* chunk, DirectionMask /*directionMask*/, Nz::UInt32 /*layerMask*/)
		{
			m_invalidatedChunks.emplace(chunk);
			*m_shouldSave = true;
		});

		auto& physicsSystem = m_world->GetSystem<Nz::Physics3DSystem>();
		m_world->AddSystem<BuoyancySystem>(*shipComponent.ship, physicsSystem.GetPhysWorld(), m_debugDrawer.get());
		m_world->AddSystem<GravityPhysicsSystem>(*shipComponent.ship, physicsSystem.GetPhysWorld());
		m_world->AddSystem<ShipSystem>();
	}

	ServerShipEnvironment::~ServerShipEnvironment()
	{
		OnSave();

		if (m_exteriorEnvironment)
		{
			// Release atmosphere from every area to the exterior atmosphere
			if (m_exteriorEnvironment && m_exteriorEntity)
			{
				auto& outsideNode = m_exteriorEntity.get<Nz::NodeComponent>();
				Nz::Vector3f outsidePosition = outsideNode.GetPosition();

				ServerAtmosphere* outsideAtmosphere = m_exteriorEnvironment->GetAtmosphereAtPosition(outsidePosition);
				if (outsideAtmosphere)
				{
					for (auto&& [chunkIndices, chunkData] : m_chunkData)
					{
						for (const auto& area : chunkData.areas->areas)
						{
							for (auto&& [gasType, amount] : area.atmosphere.GetGasAmounts().iter_kv())
								outsideAtmosphere->IncreaseGasAmount(gasType, amount);
						}
					}
				}
			}

			// Move every switchable entity out of the ship before destroying it (otherwise the entities will be destroyed)
			EnvironmentTransform outsideTransform(Nz::Vector3f::Zero(), Nz::Quaternionf::Identity());
			Nz::Vector3f outsideVelocity = Nz::Vector3f::Zero();
			if (m_exteriorEntity)
			{
				auto& outsideNode = m_exteriorEntity.get<Nz::NodeComponent>();

				outsideTransform = EnvironmentTransform(outsideNode.GetPosition(), outsideNode.GetRotation());
				outsideVelocity = m_exteriorEntity.get<Nz::RigidBody3DComponent>().GetLinearVelocity();
			}

			entt::registry& registry = m_world->GetRegistry();
			auto switchView = registry.view<Nz::NodeComponent, ClassInstanceComponent, ServerEnvironmentSwitchComponent>(entt::exclude<DatabaseComponent>);
			for (entt::entity entity : switchView)
			{
				auto& envSwitch = switchView.get<ServerEnvironmentSwitchComponent>(entity);
				envSwitch.Switch(entt::handle(registry, entity), this, m_exteriorEnvironment, outsideTransform);
			}
		}

		if (m_exteriorEntity)
			m_exteriorEntity.destroy();

		ClearEntities();

		m_world->GetRegistry().ctx().erase<ServerShipEnvironment*>();
	}

	Nz::Boxf ServerShipEnvironment::ComputeBoundingBox() const
	{
		Nz::Boxf boundingBox = Nz::Boxf::Invalid();
		auto& ship = *m_shipEntity.get<ShipComponent>().ship;
		ship.ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			Nz::Boxf chunkBox(Nz::Vector3f(chunk.GetBlockSize() * Ship::ChunkSize));
			chunkBox.Translate(ship.GetChunkOffset(chunkIndices));

			if (boundingBox.IsValid())
				boundingBox.ExtendTo(chunkBox);
			else
				boundingBox = chunkBox;
		});

		return boundingBox;
	}

	entt::handle ServerShipEnvironment::CreateEntity()
	{
		*m_shouldSave = true; //< TODO: Have a system tracking entities changes to know when this is interesting
		return ServerEnvironment::CreateEntity();
	}

	void ServerShipEnvironment::GenerateShip(bool small)
	{
		auto& blockLibrary = m_serverInstance.GetBlockLibrary();
		GetShip().Generate(blockLibrary, small);
	}

	ServerAtmosphere* ServerShipEnvironment::GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position)
	{
		// Should (almost) never be called since entities exit the ship when exiting atmosphere
		// Atmosphere is handled by multiple entities AtmosphereCarrier
		return nullptr;
	}

	const GravityController* ServerShipEnvironment::GetGravityController() const
	{
		return m_shipEntity.get<ShipComponent>().ship.get();
	}

	Ship& ServerShipEnvironment::GetShip()
	{
		return *m_shipEntity.get<ShipComponent>().ship;
	}

	const Ship& ServerShipEnvironment::GetShip() const
	{
		return *m_shipEntity.get<ShipComponent>().ship;
	}

	entt::handle ServerShipEnvironment::LinkOutsideEnvironment(ServerEnvironment* exteriorEnvironment, const EnvironmentTransform& transform)
	{
		assert(exteriorEnvironment);
		m_exteriorEnvironment = exteriorEnvironment;

		std::shared_ptr<const EntityClass> shipExteriorClass = m_serverInstance.GetEntityRegistry().FindClass("ship_exterior");
		NazaraAssert(shipExteriorClass);

		m_exteriorEntity = exteriorEnvironment->CreateEntity();
		m_exteriorEntity.emplace<Nz::NodeComponent>(transform.translation, transform.rotation);
		m_exteriorEntity.emplace<ClassInstanceComponent>(shipExteriorClass);
		m_exteriorEntity.emplace<NetworkedComponent>();
		m_exteriorEntity.emplace<ShipExteriorComponent>().ownerShip = this;

		shipExteriorClass->InitAndActivateEntity(m_exteriorEntity);

		UpdateExteriorCollider();

		return m_exteriorEntity;
	}

	Nz::Result<void, std::string> ServerShipEnvironment::Load(const nlohmann::json& data)
	{
		BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();

		auto& blockLibrary = m_serverInstance.GetBlockLibrary();
		Ship& ship = GetShip();
		try
		{
			Nz::UInt32 version = data["version"];
			if (version != 1)
				return Nz::Err(fmt::format("unhandled version {}", version));

			// Load chunks
			const nlohmann::json& chunks = data["chunks"];
			if (chunks.empty())
				return Nz::Err("no chunk in ship save");

			for (const nlohmann::json& chunkDoc : chunks)
			{
				ChunkIndices chunkIndices;
				chunkIndices.x = chunkDoc["x"];
				chunkIndices.y = chunkDoc["y"];
				chunkIndices.z = chunkDoc["z"];

				std::string chunkData = chunkDoc["chunk_data"];
				std::size_t chunkDataSize = chunkDoc["chunk_datasize"];

				using base64 = cppcodec::base64_rfc4648;
				std::vector<Nz::UInt8> compressedData = base64::decode(chunkData);
				std::vector<Nz::UInt8> decompressedData(chunkDataSize);
				std::optional compressedDataOpt = binaryCompressor.Decompress(compressedData.data(), compressedData.size(), decompressedData.data(), decompressedData.size());
				if (!compressedDataOpt)
					return Nz::Err("chunk decompression failed");

				if (*compressedDataOpt != chunkDataSize)
					return Nz::Err("chunk decompression failed (corrupt size)");

				Nz::ByteStream byteStream(decompressedData.data(), decompressedData.size());

				Chunk& chunk = ship.AddChunk(blockLibrary, chunkIndices);
				chunk.Deserialize(byteStream);
			}

			// Load entities
			if (auto it = data.find("entities"); it != data.end())
			{
				const nlohmann::json& entities = *it;

				std::size_t entityIndex = 0;
				for (const nlohmann::json& entityDoc : entities)
				{
					const std::string& uniqueId = entityDoc["unique_id"];
					const std::string& className = entityDoc["class_name"];
					Nz::UInt32 classVersion = entityDoc["class_version"];
					Nz::Vector3f position = entityDoc["position"];
					Nz::Quaternionf rotation = entityDoc["rotation"];
					const nlohmann::json& propertiesDoc = entityDoc["properties"];

					std::shared_ptr<const EntityClass> entityClass = m_serverInstance.GetEntityRegistry().FindClass(className);
					if (!entityClass)
					{
						NazaraError("Database entity #{} has unknown class {}", entityIndex, className);
						continue;
					}

					entt::handle entity = CreateEntity();
					entity.emplace<Nz::NodeComponent>(position, rotation);
					entity.emplace<NetworkedComponent>();
					entity.emplace<DatabaseComponent>(Nz::Uuid::FromString(uniqueId)); //< no planet id for ships

					entity.emplace<ClassInstanceComponent>(entityClass, entityClass->PropertiesFromJson(propertiesDoc));
					entityClass->InitAndActivateEntity(entity);

					entityIndex++;
				}
			}

			return Nz::Ok();
		}
		catch (const std::exception& e)
		{
			return Nz::Err(fmt::format("ship decoding failed: {}", e.what()));
		}
	}

	void ServerShipEnvironment::OnSave()
	{
		if (!m_playerUuid || !*m_shouldSave)
			return;

		nlohmann::json chunks = nlohmann::json::array();
		nlohmann::json entities = nlohmann::json::array();

		// Chunks
		BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();
		Nz::ByteArray byteArray;
		GetShip().ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
		{
			nlohmann::json& chunkDoc = chunks.emplace_back();
			chunkDoc["x"] = chunkIndices.x;
			chunkDoc["y"] = chunkIndices.y;
			chunkDoc["z"] = chunkIndices.z;

			byteArray.Clear();
			Nz::ByteStream byteStream(&byteArray);
			chunk.Serialize(byteStream);

			std::optional compressedDataOpt = binaryCompressor.Compress(byteArray.GetBuffer(), byteArray.GetSize());
			if NAZARA_UNLIKELY(!compressedDataOpt)
				throw std::runtime_error("chunk compression failed");

			std::span<Nz::UInt8>& compressedData = *compressedDataOpt;

			using base64 = cppcodec::base64_rfc4648;
			chunkDoc["chunk_data"] = base64::encode(compressedData.data(), compressedData.size());
			chunkDoc["chunk_datasize"] = byteArray.GetSize();
		});

		// Entities
		auto entityView = m_world->GetRegistry().view<Nz::NodeComponent, DatabaseComponent, ClassInstanceComponent>();
		for (auto&& [entity, entityNode, entityDatabase, entityClassInstance] : entityView.each())
		{
			const auto& entityClass = entityClassInstance.GetClass();

			nlohmann::json& entityDoc = entities.emplace_back();
			entityDoc["unique_id"] = entityDatabase.uniqueId.ToString();
			entityDoc["class_name"] = entityClass->GetName();
			entityDoc["class_version"] = 1; //< TODO
			entityDoc["position"] = entityNode.GetPosition();
			entityDoc["rotation"] = entityNode.GetRotation();
			entityDoc["properties"] = entityClass->PropertiesToJson(entityClassInstance.GetProperties());
		}

		bool hasEntities = !entities.empty();

		nlohmann::json shipData;
		shipData["chunks"] = std::move(chunks);
		shipData["entities"] = std::move(entities);
		shipData["version"] = Nz::UInt32(1);

		nlohmann::json body;
		body["data"] = shipData.dump();

		auto& playerToken = m_serverInstance.GetApplication().GetComponent<PlayerTokenAppComponent>();
		playerToken.QueueRequest(*m_playerUuid, Nz::WebRequestMethod::Patch, fmt::format("/v1/player_ship/{}", m_saveSlot), body, [shouldSave = m_shouldSave, uuid = *m_playerUuid, hasEntities](Nz::UInt32 code, const std::string& body)
		{
			if (code != 200)
			{
				fmt::print(fg(fmt::color::red), "failed to save player {} ship ({}): {}\n", uuid.ToString(), code, body);
				return;
			}

			if (!hasEntities)
				*shouldSave = false;
		});
	}

	void ServerShipEnvironment::OnTick(Nz::Time elapsedTime)
	{
		// Check and apply chunk areas update
		for (auto it = m_areaUpdateJobs.begin(); it != m_areaUpdateJobs.end();)
		{
			std::shared_ptr<AreaUpdateJob>& updateJob = it.value();
			if (!updateJob->isFinished)
			{
				++it;
				continue;
			}

			updateJob->applyFunc(it.key(), std::move(*updateJob));
			it = m_areaUpdateJobs.erase(it);
		}

		for (auto it = m_triggerUpdateJobs.begin(); it != m_triggerUpdateJobs.end();)
		{
			std::shared_ptr<TriggerUpdateJob>& updateJob = it.value();
			if (!updateJob->isFinished)
			{
				++it;
				continue;
			}

			updateJob->applyFunc(it.key(), std::move(*updateJob));
			it = m_triggerUpdateJobs.erase(it);
		}

		if (!m_invalidatedChunks.empty())
		{
			UpdateExteriorCollider();
			for (Chunk* chunk : m_invalidatedChunks)
				StartAreaUpdate(*chunk);

			m_invalidatedChunks.clear();
		}

		if (m_isInteriorAreaColliderInvalidated)
		{
			m_interiorAreaColliders = BuildInteriorAreaCollider();
			m_isInteriorAreaColliderGenerated = true;
			m_isInteriorAreaColliderInvalidated = false;

			OnInteriorColliderUpdated();
		}

		if (m_exteriorEnvironment && m_exteriorEntity && m_isInteriorAreaColliderGenerated) //< ensure interior finished computing before allowing entities to exit
		{
			entt::registry& registry = m_world->GetRegistry();
			auto switchView = registry.view<Nz::NodeComponent, ClassInstanceComponent, ServerEnvironmentSwitchComponent>();

			Ship& ship = GetShip();
			for (entt::entity entity : switchView)
			{
				auto& entityNode = switchView.get<Nz::NodeComponent>(entity);

				Nz::Vector3f entityPos = entityNode.GetPosition();
				Ship& ship = GetShip();

				bool isInside = false;
				for (const auto& [chunkIndices, chunkData] : m_chunkData)
				{
					if (!chunkData.hullCollider)
						continue;

					Nz::Vector3f relativePos = entityPos - ship.GetChunkOffset(chunkIndices);
					relativePos -= chunkData.hullCollider->GetCenterOfMass(); //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
					if (chunkData.hullCollider->CollisionQuery(relativePos))
					{
						isInside = true;
						break;
					}
				}

				if (isInside)
					continue;

				auto& envSwitch = switchView.get<ServerEnvironmentSwitchComponent>(entity);

				// No longer colliding with the interior
				auto& outsideNode = m_exteriorEntity.get<Nz::NodeComponent>();
				EnvironmentTransform outsideTransform(outsideNode.GetPosition(), outsideNode.GetRotation());
				Nz::Vector3f shipLinearVelocity = m_exteriorEntity.get<Nz::RigidBody3DComponent>().GetLinearVelocity();

				entt::handle newEntity = envSwitch.Switch(entt::handle(registry, entity), this, m_exteriorEnvironment, outsideTransform);
				if (Nz::PhysCharacter3DComponent* controlledCharacter = newEntity.try_get<Nz::PhysCharacter3DComponent>())
					controlledCharacter->AddLinearVelocity(shipLinearVelocity);
				else if (Nz::RigidBody3DComponent* controlledRigidbody = newEntity.try_get<Nz::RigidBody3DComponent>())
					controlledRigidbody->AddLinearVelocity(shipLinearVelocity);
			}
		}

		ServerEnvironment::OnTick(elapsedTime);
	}

	void ServerShipEnvironment::UpdateExterior(ServerEnvironment* exteriorEnvironment, entt::handle exteriorEntity)
	{
		m_exteriorEnvironment = exteriorEnvironment;
		m_exteriorEntity = exteriorEntity;
	}

	void ServerShipEnvironment::StartAreaUpdate(const Chunk& chunk)
	{
		// Try to cancel current update job to avoid useless work
		if (auto it = m_areaUpdateJobs.find(chunk.GetIndices()); it != m_areaUpdateJobs.end())
		{
			AreaUpdateJob& job = *it->second;
			job.isCancelled = true;
		}

		auto& app = m_serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		std::shared_ptr<AreaUpdateJob> updateJob = std::make_shared<AreaUpdateJob>();

		updateJob->applyFunc = [this, chunkPtr = chunk.shared_from_this()](const ChunkIndices& chunkIndices, AreaUpdateJob&& updateJob)
		{
			assert(m_chunkData.contains(chunkIndices));
			auto& chunkData = m_chunkData[chunkIndices];

			if (chunkData.areas)
			{
				for (Area& area : chunkData.areas->areas)
				{
					if (area.atmosphereEntity)
						area.atmosphereEntity.destroy();
				}
			}

			std::size_t areaCount = updateJob.chunkArea->areas.size();

			if (areaCount > 0)
			{
				// Take atmosphere from exterior from newly created/extended areas
				if (m_exteriorEnvironment && m_exteriorEntity)
				{
					auto& outsideNode = m_exteriorEntity.get<Nz::NodeComponent>();
					Nz::Vector3f outsidePosition = outsideNode.GetPosition();

					ServerAtmosphere* outsideAtmosphere = m_exteriorEnvironment->GetAtmosphereAtPosition(outsidePosition);
					if (outsideAtmosphere)
					{
						for (std::size_t areaIndex = 0; areaIndex < areaCount; ++areaIndex)
						{
							std::size_t occupiedBlockCount;
							if (updateJob.previousAreaList)
								occupiedBlockCount = updateJob.previousOutsideAreaOccupancy.areaOccupancy[areaIndex];
							else
								occupiedBlockCount = updateJob.chunkArea->areas[areaIndex].blocks.Count();

							ServerAtmosphere& newAtmosphere = updateJob.chunkArea->areas[areaIndex].atmosphere;

							Nz::UInt64 oxygenAmount = Constants::SecondsToEmptyOxygenBlock * Nz::UInt64(Constants::PlayerOxygenConsumption) * occupiedBlockCount;
							Nz::UInt64 nitrogenAmount = oxygenAmount * (100 - Constants::OxygenAtmospherePct) / Constants::OxygenAtmospherePct;
							outsideAtmosphere->DecreaseGasAmount(GasType::Oxygen, oxygenAmount);
							outsideAtmosphere->DecreaseGasAmount(GasType::Nitrogen, nitrogenAmount);
							newAtmosphere.IncreaseGasAmount(GasType::Oxygen, oxygenAmount);
							newAtmosphere.IncreaseGasAmount(GasType::Nitrogen, nitrogenAmount);
						}
					}
				}

				std::size_t previousAreaCount = (updateJob.previousAreaList) ? updateJob.previousAreaList->areas.size() : 0;

				// Split/join atmosphere values from previous areas
				for (std::size_t previousAreaIndex = 0; previousAreaIndex < previousAreaCount; ++previousAreaIndex)
				{
					std::size_t blockCount = updateJob.previousAreaList->areas[previousAreaIndex].blocks.Count();
					const ServerAtmosphere& previousAtmosphere = updateJob.previousAreaList->areas[previousAreaIndex].atmosphere;

					// Attribute a part of the previous area atmosphere to each new area
					Nz::EnumArray<GasType, Nz::UInt64> leftOvers;
					leftOvers.fill(0);

					for (std::size_t areaIndex = 0; areaIndex < areaCount; ++areaIndex)
					{
						std::size_t occupiedBlockCount = updateJob.previousAreaOccupancy[areaIndex].areaOccupancy[previousAreaIndex];
						ServerAtmosphere& newAtmosphere = updateJob.chunkArea->areas[areaIndex].atmosphere;

						for (auto&& [gasType, amount] : previousAtmosphere.GetGasAmounts().iter_kv())
						{
							Nz::UInt64 gasAmount = amount * occupiedBlockCount / blockCount;
							newAtmosphere.IncreaseGasAmount(gasType, gasAmount);
							leftOvers[gasType] += amount * occupiedBlockCount - gasAmount * blockCount;
						}
					}

					// Ensure we don't lose atmosphere because of integer division
					for (auto&& [gasType, leftOver] : leftOvers.iter_kv())
						updateJob.chunkArea->areas[0].atmosphere.IncreaseGasAmount(gasType, leftOver);
				}
			}

			chunkData.areas = std::move(updateJob.chunkArea);

			StartTriggerUpdate(*chunkPtr, chunkData.areas);
		};

		assert(m_chunkData.contains(chunk.GetIndices()));
		ChunkData& chunkData = m_chunkData[chunk.GetIndices()];

		taskScheduler.AddTask([updateJob, previousAreaList = chunkData.areas, chunkPtr = chunk.shared_from_this()]
		{
			updateJob->previousAreaList = previousAreaList;

			chunkPtr->LockRead();
			updateJob->chunkArea = GenerateChunkAreas(*chunkPtr, updateJob->isCancelled);
			chunkPtr->UnlockRead();

			// Compute previous area occupancy
			if (updateJob->chunkArea && previousAreaList)
			{
				std::size_t areaCount = updateJob->chunkArea->areas.size();
				std::size_t previousAreaCount = previousAreaList->areas.size();

				Nz::Bitset<Nz::UInt64> commonBlocks;

				updateJob->previousAreaOccupancy.resize(areaCount);
				updateJob->previousOutsideAreaOccupancy.areaOccupancy.resize(areaCount);
				for (std::size_t areaIndex = 0; areaIndex < areaCount; ++areaIndex)
				{
					auto& occupancy = updateJob->previousAreaOccupancy[areaIndex];
					occupancy.areaOccupancy.resize(previousAreaCount);

					for (std::size_t previousAreaIndex = 0; previousAreaIndex < previousAreaCount; ++previousAreaIndex)
					{
						if (updateJob->isCancelled)
							return;

						// Occupancy = countbits(blocks & prevBlocks)
						commonBlocks.PerformsAND(updateJob->chunkArea->areas[areaIndex].blocks, previousAreaList->areas[previousAreaIndex].blocks);
						occupancy.areaOccupancy[previousAreaIndex] = commonBlocks.Count();
					}

					if (previousAreaList->outsideArea)
					{
						commonBlocks.PerformsAND(updateJob->chunkArea->areas[areaIndex].blocks, previousAreaList->outsideArea->blocks);
						updateJob->previousOutsideAreaOccupancy.areaOccupancy[areaIndex] = commonBlocks.Count();
					}
				}
			}

			updateJob->isFinished = true;
		});

		m_areaUpdateJobs.insert_or_assign(chunk.GetIndices(), std::move(updateJob));
	}

	void ServerShipEnvironment::StartTriggerUpdate(const Chunk& chunk, std::shared_ptr<AreaList> areaList)
	{
		// Try to cancel current update job to avoid useless work
		if (auto it = m_triggerUpdateJobs.find(chunk.GetIndices()); it != m_triggerUpdateJobs.end())
		{
			TriggerUpdateJob& job = *it->second;
			job.isCancelled = true;
		}

		auto& app = m_serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		std::shared_ptr<TriggerUpdateJob> updateJob = std::make_shared<TriggerUpdateJob>();

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, TriggerUpdateJob&& updateJob)
		{
			assert(m_chunkData.contains(chunkIndices));
			auto& chunkData = m_chunkData[chunkIndices];
			chunkData.areaCollider = std::move(updateJob.interiorCollider);
			chunkData.hullCollider = std::move(updateJob.hullCollider);
			m_isInteriorAreaColliderInvalidated = true;

			if (!chunkData.areaCollider)
				return;

			for (Area& area : chunkData.areas->areas)
			{
				if (area.boundingBoxes.empty())
					continue;

				area.atmosphereEntity = CreateEntity();
				area.atmosphereEntity.emplace<Nz::NodeComponent>();
				auto& atmosphereCarrier = area.atmosphereEntity.emplace<AtmosphereCarrier>();
				atmosphereCarrier.atmosphere = &area.atmosphere;
				atmosphereCarrier.collider = area.collider;
				atmosphereCarrier.aabb = area.boundingBoxes[0];
				for (std::size_t i = 1; i < area.boundingBoxes.size(); ++i)
					atmosphereCarrier.aabb.ExtendTo(area.boundingBoxes[i]);
			}

			if (m_debugDrawer)
			{
				std::size_t areaIndex = 0;
				std::size_t seed = std::hash<void*>{}(this);
				std::minstd_rand colorGen(seed);
				std::uniform_real_distribution<float> dis(0.f, 360.f);
				for (const Area& area : chunkData.areas->areas)
				{
					std::vector<Nz::UInt16> indices;
					std::vector<Nz::Vector3f> positions;
					area.collider->BuildDebugMesh(positions, indices, Nz::Matrix4f::Translate(GetShip().GetChunkOffset(chunkIndices)));

					m_debugDrawer->DrawLines(seed + areaIndex, 5.f, indices, positions, Nz::Color::FromHSV(dis(colorGen), 1.f, 1.f));
					areaIndex++;
				}
			}
		};

		taskScheduler.AddTask([areaList, updateJob, chunkPtr = chunk.shared_from_this()]
		{
			chunkPtr->LockRead();
			for (Area& area : areaList->areas)
			{
				if (updateJob->isCancelled)
				{
					chunkPtr->UnlockRead(); // TODO: lock guards
					return;
				}

				BuildAreaCollider(area, *chunkPtr);
			}

			updateJob->interiorCollider = BuildTriggerCollider(*chunkPtr, *areaList, Nz::Vector3f::Zero(), updateJob->isCancelled);
			updateJob->hullCollider = BuildTriggerCollider(*chunkPtr, *areaList, Nz::Vector3f(chunkPtr->GetBlockSize() * 2.f), updateJob->isCancelled);
			chunkPtr->UnlockRead();

			updateJob->isFinished = true;
		});

		m_triggerUpdateJobs.insert_or_assign(chunk.GetIndices(), std::move(updateJob));
	}

	std::shared_ptr<Nz::Collider3D> ServerShipEnvironment::BuildInteriorAreaCollider()
	{
		if (m_chunkData.empty())
			return nullptr;

		// Handle for common case
		if (m_chunkData.size() == 1 && m_chunkData.begin().key() == ChunkIndices(0, 0, 0))
			return m_chunkData.begin().value().areaCollider;

		Ship& ship = GetShip();

		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;
		for (const auto& [chunkIndices, chunkData] : m_chunkData)
		{
			if (!chunkData.areaCollider)
				continue;

			auto& childCollider = childColliders.emplace_back();
			childCollider.collider = chunkData.areaCollider;
			childCollider.offset = ship.GetChunkOffset(chunkIndices);
		}

		if (childColliders.empty())
			return nullptr;

		return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	void ServerShipEnvironment::UpdateExteriorCollider()
	{
		// TODO: Move to ship_exterior class

		if (!m_exteriorEntity)
			return;

		auto& ship = GetShip();

		std::size_t fullBlockCount = 0;
		ship.ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
		{
			fullBlockCount += chunk.GetCollisionCellMask(0).Count();
		});

		auto& rigidBody = m_exteriorEntity.get<Nz::RigidBody3DComponent>();
		rigidBody.SetCollider(ship.BuildHullCollider());
		rigidBody.SetMass(fullBlockCount);
	}

	auto ServerShipEnvironment::BuildArea(const Chunk& chunk, std::size_t firstBlockIndex, Nz::Bitset<Nz::UInt64>& remainingBlocks) -> Area
	{
		Nz::Bitset<Nz::UInt64> areaBlocks(ShipChunkBlockCount, false);

		std::vector<std::size_t> candidateBlocks;
		candidateBlocks.push_back(firstBlockIndex);

		while (!candidateBlocks.empty())
		{
			std::size_t blockIndex = candidateBlocks.back();
			candidateBlocks.pop_back();

			areaBlocks[blockIndex] = true;

			remainingBlocks[blockIndex] = false;

			Nz::Vector3ui chunkSize = chunk.GetSize();

			BlockIndex block = chunk.GetBlockContent(blockIndex);
			bool isEmpty = block == EmptyBlockIndex;

			auto AddCandidateBlock = [&](const Nz::Vector3ui& blockIndices)
			{
				std::size_t blockIndex = chunk.GetBlockLocalIndex(blockIndices);
				if (remainingBlocks[blockIndex])
				{
					if (!isEmpty)
					{
						// Non-empty blocks can look at other non-empty blocks
						if (chunk.GetBlockContent(blockIndex) != EmptyBlockIndex)
							candidateBlocks.push_back(blockIndex);
					}
					else
						candidateBlocks.push_back(blockIndex);
				}
			};

			Nz::Vector3ui blockIndices = chunk.GetBlockLocalIndices(blockIndex);
			for (int zOffset = -1; zOffset <= 1; ++zOffset)
			{
				for (int yOffset = -1; yOffset <= 1; ++yOffset)
				{
					for (int xOffset = -1; xOffset <= 1; ++xOffset)
					{
						if (xOffset == 0 && yOffset == 0 && zOffset == 0)
							continue;

						Nz::Vector3i candidateIndicesSigned = Nz::Vector3i(blockIndices);
						candidateIndicesSigned.x += xOffset;
						candidateIndicesSigned.y += yOffset;
						candidateIndicesSigned.z += zOffset;

						Nz::Vector3ui candidateIndices = Nz::Vector3ui(candidateIndicesSigned);
						if (candidateIndices.x < chunkSize.x && candidateIndices.y < chunkSize.y && candidateIndices.z < chunkSize.z)
							AddCandidateBlock(candidateIndices);
					}
				}
			}
		}

		Area area;
		area.blocks = std::move(areaBlocks);

		return area;
	}

	void ServerShipEnvironment::BuildAreaCollider(Area& area, const Chunk& chunk)
	{
		area.boundingBoxes.clear();
		FlatChunk::BuildCollider(chunk.GetSize(), area.blocks, [&](const Nz::Boxf& box)
		{
			area.boundingBoxes.push_back(box);
		});

		if (area.boundingBoxes.empty())
			return;

		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;
		for (const Nz::Boxf& box : area.boundingBoxes)
		{
			auto& childCollider = childColliders.emplace_back();
			childCollider.offset = box.GetCenter() * chunk.GetBlockSize();
			childCollider.collider = std::make_shared<Nz::BoxCollider3D>(box.GetLengths() * chunk.GetBlockSize());
		}

		area.collider = std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	std::shared_ptr<Nz::Collider3D> ServerShipEnvironment::BuildTriggerCollider(const Chunk& chunk, const AreaList& areaList, const Nz::Vector3f& sizeMargin, std::atomic_bool& isCancelled)
	{
		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;
		for (const Area& area : areaList.areas)
		{
			for (const Nz::Boxf& box : area.boundingBoxes)
			{
				auto& childCollider = childColliders.emplace_back();
				childCollider.offset = box.GetCenter() * chunk.GetBlockSize();
				childCollider.collider = std::make_shared<Nz::BoxCollider3D>(box.GetLengths() * chunk.GetBlockSize() + sizeMargin);
			}
		}

		if (childColliders.empty())
			return nullptr;

		return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	auto ServerShipEnvironment::GenerateChunkAreas(const Chunk& chunk, std::atomic_bool& isCancelled) -> std::shared_ptr<AreaList>
	{
		Nz::Bitset<Nz::UInt64> remainingBlocks(ShipChunkBlockCount, true);

		// Find first candidate (= a random empty block)
		auto FindFirstCandidate = [&]
		{
			const Nz::Bitset<Nz::UInt64>& collisionCellMask = chunk.GetCollisionCellMask(0);
			for (std::size_t i = 0; i < collisionCellMask.GetBlockCount(); ++i)
			{
				Nz::UInt64 mask = collisionCellMask.GetBlock(i);
				mask = ~mask;

				unsigned int fsb = Nz::FindFirstBit(mask);
				if (fsb != 0)
				{
					unsigned int localBlockIndex = i * Nz::BitCount<Nz::UInt64> + fsb - 1;
					return localBlockIndex;
				}
			}

			return Nz::MaxValue<unsigned int>();
		};

		std::shared_ptr<AreaList> chunkArea = std::make_shared<AreaList>();

		unsigned int firstCandidate = FindFirstCandidate();
		if (firstCandidate != Nz::MaxValue<unsigned int>())
		{
			if (isCancelled)
				return {};

			chunkArea->outsideArea = BuildArea(chunk, firstCandidate, remainingBlocks);

			while (remainingBlocks.TestAny())
			{
				if (isCancelled)
					return {};

				chunkArea->areas.push_back(BuildArea(chunk, remainingBlocks.FindFirst(), remainingBlocks));
			}
		}

		return chunkArea;
	}
}
