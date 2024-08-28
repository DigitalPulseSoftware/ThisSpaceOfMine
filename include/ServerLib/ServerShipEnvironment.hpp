// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

namespace Nz
{
	class Collider3D;
}

namespace tsom
{
	class ChunkEntities;
	class ServerPlayer;
	class Ship;

	class TSOM_SERVERLIB_API ServerShipEnvironment final : public ServerEnvironment
	{
		public:
			ServerShipEnvironment(ServerInstance& serverInstance, const std::optional<Nz::Uuid>& playerUuid, int saveSlot);
			ServerShipEnvironment(const ServerShipEnvironment&) = delete;
			ServerShipEnvironment(ServerShipEnvironment&&) = delete;
			~ServerShipEnvironment();

			entt::handle CreateEntity() override;

			void GenerateShip(bool small);

			const GravityController* GetGravityController() const override;
			Ship& GetShip();
			const Ship& GetShip() const;
			inline entt::handle GetShipEntity() const;

			entt::handle LinkOutsideEnvironment(ServerEnvironment* environment, const EnvironmentTransform& transform);

			Nz::Result<void, std::string> Load(const nlohmann::json& data);

			void OnSave() override;
			void OnTick(Nz::Time elapsedTime) override;

			ServerShipEnvironment& operator=(const ServerShipEnvironment&) = delete;
			ServerShipEnvironment& operator=(ServerShipEnvironment&&) = delete;

		private:
			struct Area;
			struct AreaList;

			void StartAreaUpdate(const Chunk* chunk);
			void StartTriggerUpdate(const Chunk* chunk, std::shared_ptr<AreaList> areaList);

			std::shared_ptr<Nz::Collider3D> BuildCombinedAreaCollider();
			void UpdateProxyCollider();

			static Area BuildArea(const Chunk* chunk, std::size_t firstBlockIndex, Nz::Bitset<Nz::UInt64>& remainingBlocks);
			static std::shared_ptr<Nz::Collider3D> BuildTriggerCollider(const Chunk* chunk, const AreaList& areaList, const Nz::Vector3f& sizeMargin, std::atomic_bool& isCancelled);
			static std::shared_ptr<AreaList> GenerateChunkAreas(const Chunk* chunk, std::atomic_bool& isCancelled);

			struct Area
			{
				Nz::Bitset<Nz::UInt64> blocks;
			};

			struct AreaList
			{
				std::vector<Area> areas;
			};

			struct ChunkData
			{
				std::shared_ptr<Nz::Collider3D> areaCollider;
				std::shared_ptr<Nz::Collider3D> expandedAreaCollider;
				std::shared_ptr<AreaList> areas;
				float blockSize;
			};

			struct UpdateJob
			{
				std::atomic_bool isCancelled = false;
				std::atomic_bool isFinished = false;
			};

			struct AreaUpdateJob : UpdateJob
			{
				std::function<void(ChunkIndices chunkIndices, AreaUpdateJob&& updateJob)> applyFunc;
				std::shared_ptr<AreaList> chunkArea;
			};

			struct TriggerUpdateJob : UpdateJob
			{
				std::function<void(ChunkIndices chunkIndices, TriggerUpdateJob&& updateJob)> applyFunc;
				std::shared_ptr<Nz::Collider3D> collider;
				std::shared_ptr<Nz::Collider3D> expandedCollider;
			};

			entt::handle m_proxyEntity;
			entt::handle m_shipEntity;
			std::optional<Nz::Uuid> m_playerUuid;
			std::shared_ptr<Nz::Collider3D> m_combinedAreaColliders;
			tsl::hopscotch_map<ChunkIndices, std::shared_ptr<AreaUpdateJob>> m_areaUpdateJobs;
			tsl::hopscotch_map<ChunkIndices, std::shared_ptr<TriggerUpdateJob>> m_triggerUpdateJobs;
			tsl::hopscotch_map<ChunkIndices, ChunkData> m_chunkData;
			tsl::hopscotch_set<Chunk*> m_invalidatedChunks;
			ServerEnvironment* m_outsideEnvironment;
			bool m_isCombinedAreaColliderInvalidated;
			bool m_shouldSave;
			int m_saveSlot;
	};
}

#include <ServerLib/ServerShipEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP
