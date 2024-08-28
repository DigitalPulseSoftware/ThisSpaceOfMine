// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerShipEnvironment.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Systems/ShipSystem.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fmt/color.h>
#include <fmt/std.h>
#include <nlohmann/json.hpp>

namespace tsom
{
	namespace
	{
		constexpr unsigned int ShipChunkBlockCount = Ship::ChunkSize * Ship::ChunkSize * Ship::ChunkSize;
	}

	ServerShipEnvironment::ServerShipEnvironment(ServerInstance& serverInstance, const std::optional<Nz::Uuid>& playerUuid, int saveSlot) :
	ServerEnvironment(serverInstance),
	m_playerUuid(playerUuid),
	m_outsideEnvironment(nullptr),
	m_isCombinedAreaColliderInvalidated(false),
	m_shouldSave(false),
	m_saveSlot(saveSlot)
	{
		auto& app = serverInstance.GetApplication();
		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_shipEntity = CreateEntity();
		m_shipEntity.emplace<Nz::NodeComponent>();
		m_shipEntity.emplace<NetworkedComponent>();

		auto& shipComponent = m_shipEntity.emplace<ShipComponent>(1.f);

		shipComponent.shipEntities = std::make_unique<ChunkEntities>(app, m_world, shipComponent, blockLibrary);
		shipComponent.shipEntities->SetParentEntity(m_shipEntity);

		m_world.AddSystem<ShipSystem>();

		shipComponent.OnChunkAdded.Connect([this](ChunkContainer*, Chunk* chunk)
		{
			auto& chunkData = m_chunkData[chunk->GetIndices()];
			chunkData.blockSize = chunk->GetBlockSize();
			m_invalidatedChunks.emplace(chunk);
		});

		shipComponent.OnChunkRemove.Connect([this](ChunkContainer*, Chunk* chunk)
		{
			const ChunkIndices& indices = chunk->GetIndices();
			m_chunkData.erase(indices);

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

		shipComponent.OnChunkUpdated.Connect([this](ChunkContainer*, Chunk* chunk, DirectionMask)
		{
			m_invalidatedChunks.emplace(chunk);
			m_shouldSave = true;
		});
	}

	ServerShipEnvironment::~ServerShipEnvironment()
	{
		OnSave();

		if (m_proxyEntity)
			m_proxyEntity.destroy();

		m_shipEntity.destroy();
	}

	entt::handle ServerShipEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	void ServerShipEnvironment::GenerateShip(bool small)
	{
		auto& blockLibrary = m_serverInstance.GetBlockLibrary();
		GetShip().Generate(blockLibrary, small);
	}

	const GravityController* ServerShipEnvironment::GetGravityController() const
	{
		return &m_shipEntity.get<ShipComponent>();
	}

	Ship& ServerShipEnvironment::GetShip()
	{
		return m_shipEntity.get<ShipComponent>();
	}

	const Ship& ServerShipEnvironment::GetShip() const
	{
		return m_shipEntity.get<ShipComponent>();
	}

	entt::handle ServerShipEnvironment::LinkOutsideEnvironment(ServerEnvironment* outsideEnvironment, const EnvironmentTransform& transform)
	{
		assert(outsideEnvironment);
		m_outsideEnvironment = outsideEnvironment;

		outsideEnvironment->Connect(*this, transform);
		Connect(*outsideEnvironment, -transform);

		m_proxyEntity = outsideEnvironment->CreateEntity();
		auto& proxyNode = m_proxyEntity.emplace<Nz::NodeComponent>(transform.translation, transform.rotation);

		Nz::RigidBody3D::DynamicSettings physSettings(GetShip().BuildHullCollider(), 100.f);
		physSettings.objectLayer = Constants::ObjectLayerDynamic;
		physSettings.linearDamping = 0.f;

		m_proxyEntity.emplace<Nz::RigidBody3DComponent>(physSettings);

		auto& envProxy = m_proxyEntity.emplace<EnvironmentProxyComponent>();
		envProxy.fromEnv = outsideEnvironment;
		envProxy.toEnv = this;

		auto& shipEntry = m_proxyEntity.emplace<EnvironmentEnterTriggerComponent>();
		shipEntry.entryTrigger = m_combinedAreaColliders;
		if (m_combinedAreaColliders)
			shipEntry.aabb = m_combinedAreaColliders->GetBoundingBox();

		shipEntry.targetEnvironment = this;

		return m_proxyEntity;
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

			const nlohmann::json& chunks = data["chunks"];
			if (chunks.empty())
				return Nz::Err("no chunk in ship save");

			for (const nlohmann::json& chunkDoc : chunks)
			{
				ChunkIndices chunkIndices;
				chunkIndices.x = chunkDoc["x"];
				chunkIndices.y = chunkDoc["y"];
				chunkIndices.z = chunkDoc["z"];

				std::string_view chunkData = chunkDoc["chunk_data"];
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

			return Nz::Ok();
		}
		catch (const std::exception& e)
		{
			return Nz::Err(fmt::format("ship decoding failed: {}", e.what()));
		}
	}

	void ServerShipEnvironment::OnSave()
	{
		if (!m_playerUuid || !m_shouldSave)
			return;

		nlohmann::json chunks;

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

		nlohmann::json shipData;
		shipData["chunks"] = std::move(chunks);
		shipData["version"] = Nz::UInt32(1);

		nlohmann::json body;
		body["data"] = shipData.dump();

		auto& playerToken = m_serverInstance.GetApplication().GetComponent<PlayerTokenAppComponent>();
		playerToken.QueueRequest(*m_playerUuid, Nz::WebRequestMethod::Patch, fmt::format("/v1/player_ship/{}", m_saveSlot), body, [this, uuid = *m_playerUuid](Nz::UInt32 code, const std::string& body)
		{
			if (code == 200)
				m_shouldSave = false;
			else
				fmt::print(fg(fmt::color::red), "failed to save player {} ship ({}): {}\n", uuid.ToString(), code, body);
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
			UpdateProxyCollider();
			for (Chunk* chunk : m_invalidatedChunks)
				StartAreaUpdate(chunk);

			m_invalidatedChunks.clear();
		}

		if (m_isCombinedAreaColliderInvalidated)
		{
			m_combinedAreaColliders = BuildCombinedAreaCollider();
			m_isCombinedAreaColliderInvalidated = false;

			if (m_proxyEntity)
			{
				auto& shipEntry = m_proxyEntity.get<EnvironmentEnterTriggerComponent>();
				shipEntry.entryTrigger = m_combinedAreaColliders;
				if (shipEntry.entryTrigger)
					shipEntry.aabb = m_combinedAreaColliders->GetBoundingBox();
			}
		}

		if (m_outsideEnvironment)
		{
			ForEachPlayer([this](ServerPlayer& player)
			{
				entt::handle controlledEntity = player.GetControlledEntity();
				if (!controlledEntity)
					return;

				if (player.GetControlledEntityEnvironment() == this)
				{
					Nz::Vector3f playerPos = controlledEntity.get<Nz::NodeComponent>().GetPosition();
					Ship& ship = GetShip();

					for (const auto& [chunkIndices, chunkData] : m_chunkData)
					{
						if (!chunkData.expandedAreaCollider)
							continue;

						Nz::Vector3f relativePos = playerPos - ship.GetChunkOffset(chunkIndices);
						if (chunkData.expandedAreaCollider->CollisionQuery(relativePos))
							return;
					}

					// No longer colliding with the interior
					player.MoveEntityToEnvironment(m_outsideEnvironment);
				}
			});
		}

		ServerEnvironment::OnTick(elapsedTime);
	}

	void ServerShipEnvironment::StartAreaUpdate(const Chunk* chunk)
	{
		// Try to cancel current update job to avoid useless work
		if (auto it = m_areaUpdateJobs.find(chunk->GetIndices()); it != m_areaUpdateJobs.end())
		{
			AreaUpdateJob& job = *it->second;
			job.isCancelled = true;
		}

		auto& app = m_serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		std::shared_ptr<AreaUpdateJob> updateJob = std::make_shared<AreaUpdateJob>();

		updateJob->applyFunc = [this, chunk](const ChunkIndices& chunkIndices, AreaUpdateJob&& updateJob)
		{
			assert(m_chunkData.contains(chunkIndices));
			auto& chunkData = m_chunkData[chunkIndices];
			chunkData.areas = std::move(updateJob.chunkArea);
			StartTriggerUpdate(chunk, chunkData.areas);
		};

		taskScheduler.AddTask([chunk, updateJob]
		{
			chunk->LockRead();
			updateJob->chunkArea = GenerateChunkAreas(chunk, updateJob->isCancelled);
			chunk->UnlockRead();

			updateJob->isFinished = true;
		});

		m_areaUpdateJobs.insert_or_assign(chunk->GetIndices(), std::move(updateJob));
	}

	void ServerShipEnvironment::StartTriggerUpdate(const Chunk* chunk, std::shared_ptr<AreaList> areaList)
	{
		// Try to cancel current update job to avoid useless work
		if (auto it = m_triggerUpdateJobs.find(chunk->GetIndices()); it != m_triggerUpdateJobs.end())
		{
			TriggerUpdateJob& job = *it->second;
			job.isCancelled = true;
		}

		auto& app = m_serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		assert(m_chunkData.contains(chunk->GetIndices()));
		ChunkData& chunkData = m_chunkData[chunk->GetIndices()];

		std::shared_ptr<TriggerUpdateJob> updateJob = std::make_shared<TriggerUpdateJob>();

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, TriggerUpdateJob&& updateJob)
		{
			assert(m_chunkData.contains(chunkIndices));
			auto& chunkData = m_chunkData[chunkIndices];
			chunkData.areaCollider = std::move(updateJob.collider);
			chunkData.expandedAreaCollider = std::move(updateJob.expandedCollider);
			m_isCombinedAreaColliderInvalidated = true;

			if (!chunkData.areaCollider)
				return;

			Packets::DebugDrawLineList debugDrawLineList;
			debugDrawLineList.color = Nz::Color::Blue();
			debugDrawLineList.duration = 5.f;
			debugDrawLineList.position = GetShip().GetChunkOffset(chunkIndices);
			debugDrawLineList.rotation = Nz::Quaternionf::Identity();
			chunkData.areaCollider->BuildDebugMesh(debugDrawLineList.vertices, debugDrawLineList.indices, Nz::Matrix4f::Identity());

			m_serverInstance.ForEachPlayer([&](ServerPlayer& player)
			{
				auto* session = player.GetSession();
				if (!session)
					return;

				auto& visibility = player.GetVisibilityHandler();

				debugDrawLineList.environmentId = visibility.GetEnvironmentId(this);
				session->SendPacket(debugDrawLineList);
			});
		};

		taskScheduler.AddTask([areaList, chunk, updateJob]
		{
			chunk->LockRead();
			updateJob->collider = BuildTriggerCollider(chunk, *areaList, Nz::Vector3f::Zero(), updateJob->isCancelled);
			updateJob->expandedCollider = BuildTriggerCollider(chunk, *areaList, Nz::Vector3f(2.f), updateJob->isCancelled);
			chunk->UnlockRead();

			updateJob->isFinished = true;
		});

		m_triggerUpdateJobs.insert_or_assign(chunk->GetIndices(), std::move(updateJob));
	}

	std::shared_ptr<Nz::Collider3D> ServerShipEnvironment::BuildCombinedAreaCollider()
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

	void ServerShipEnvironment::UpdateProxyCollider()
	{
		if (!m_proxyEntity)
			return;

		auto& ship = GetShip();
		std::shared_ptr<Nz::Collider3D> hullCollider = ship.BuildHullCollider();
		std::size_t fullBlockCount = 0;
		ship.ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
		{
			fullBlockCount += chunk.GetCollisionCellMask().Count();
		});

		auto& rigidBody = m_proxyEntity.get<Nz::RigidBody3DComponent>();
		rigidBody.SetGeom(std::move(hullCollider));
		rigidBody.SetMass(fullBlockCount);
	}

	auto ServerShipEnvironment::BuildArea(const Chunk* chunk, std::size_t firstBlockIndex, Nz::Bitset<Nz::UInt64>& remainingBlocks) -> Area
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

			Nz::Vector3ui chunkSize = chunk->GetSize();

			BlockIndex block = chunk->GetBlockContent(blockIndex);
			bool isEmpty = block == EmptyBlockIndex;

			auto AddCandidateBlock = [&](const Nz::Vector3ui& blockIndices)
			{
				std::size_t blockIndex = chunk->GetBlockLocalIndex(blockIndices);
				if (remainingBlocks[blockIndex])
				{
					if (!isEmpty)
					{
						// Non-empty blocks can look at other non-empty blocks
						if (chunk->GetBlockContent(blockIndex) != EmptyBlockIndex)
							candidateBlocks.push_back(blockIndex);
					}
					else
						candidateBlocks.push_back(blockIndex);
				}
			};

			Nz::Vector3ui blockIndices = chunk->GetBlockLocalIndices(blockIndex);
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

	std::shared_ptr<Nz::Collider3D> ServerShipEnvironment::BuildTriggerCollider(const Chunk* chunk, const AreaList& areaList, const Nz::Vector3f& sizeMargin, std::atomic_bool& isCancelled)
	{
		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;
		for (const Area& area : areaList.areas)
		{
			if (isCancelled)
				return nullptr;

			auto AddBox = [&](const Nz::Boxf& box)
			{
				auto& childCollider = childColliders.emplace_back();
				childCollider.offset = box.GetCenter();
				childCollider.collider = std::make_shared<Nz::BoxCollider3D>(box.GetLengths() + sizeMargin);
			};

			FlatChunk::BuildCollider(chunk->GetSize(),  area.blocks, AddBox);
		}

		if (childColliders.empty())
			return nullptr;

		return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	auto ServerShipEnvironment::GenerateChunkAreas(const Chunk* chunk, std::atomic_bool& isCancelled) -> std::shared_ptr<AreaList>
	{
		Nz::HighPrecisionClock clock;

		Nz::Bitset<Nz::UInt64> remainingBlocks(ShipChunkBlockCount, true);

		// Find first candidate (= a random empty block)
		auto FindFirstCandidate = [&]
		{
			const Nz::Bitset<Nz::UInt64>& collisionCellMask = chunk->GetCollisionCellMask();
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

		std::size_t firstCandidate = FindFirstCandidate();
		if (firstCandidate != Nz::MaxValue<std::size_t>())
		{
			if (isCancelled)
				return {};

			Area outside = BuildArea(chunk, firstCandidate, remainingBlocks);

			while (remainingBlocks.TestAny())
			{
				if (isCancelled)
					return {};

				chunkArea->areas.push_back(BuildArea(chunk, remainingBlocks.FindFirst(), remainingBlocks));
			}

			fmt::print("integrity check took {}\n", fmt::streamed(clock.GetElapsedTime()));

			fmt::print("{} area found\n", chunkArea->areas.size());
		}

		return chunkArea;
	}
}
