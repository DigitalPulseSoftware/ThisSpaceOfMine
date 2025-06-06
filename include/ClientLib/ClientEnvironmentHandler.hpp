// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTENVIRONMENTHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTENVIRONMENTHANDLER_HPP

#include <ClientLib/Export.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/Angle.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>

namespace Nz
{
	class ApplicationBase;
	class Collider3D;
	class DebugDrawer;
	class DirectionalLight;
	class EnttWorld;
	class ImGuiPlugin;
	class MaterialInstance;
	class Model;
	class Node;
}

namespace tsom
{
	class ClientBlockLibrary;
	class GravityController;
	class ImGuiRuntime;
	struct PlayerAnimationAssets;

	class TSOM_CLIENTLIB_API ClientEnvironmentHandler
	{
		public:
			ClientEnvironmentHandler(Nz::ApplicationBase& app, ClientSessionHandler& sessionHandler, Nz::EnttWorld& world, entt::handle cameraEntity, ClientBlockLibrary& blockLibrary);
			ClientEnvironmentHandler(const ClientEnvironmentHandler&) = delete;
			ClientEnvironmentHandler(ClientEnvironmentHandler&&) = delete;
			~ClientEnvironmentHandler();

			void Draw(Nz::Time elapsedTime, Nz::DebugDrawer* debugDrawer);

			inline entt::handle GetControlledEntity() const;
			inline const GravityController* GetGravityController(std::size_t environmentIndex) const;
			inline ScriptingContext& GetScriptingContext();

			void LoadScripts(bool isReloading = false);

			void Update(Nz::Time elapsedTime, ImGuiRuntime* imguiRuntime);

			ClientEnvironmentHandler& operator=(const ClientEnvironmentHandler&) = delete;
			ClientEnvironmentHandler& operator=(ClientEnvironmentHandler&&) = delete;

			NazaraSignal(OnControlledEntityChanged, entt::handle /*newEntity*/);
			NazaraSignal(OnControlledEntityStateUpdate, InputIndex /*lastInputIndex*/, const Packets::S_EntitiesStateUpdate::ControlledCharacter& /*characterData*/);
			NazaraSignal(OnPilotShip, entt::handle /*controlledShip*/, entt::handle /*controlledShipExterior*/, const Nz::Quaternionf& /*referenceRotation*/);
			NazaraSignal(OnStopPilotingShip);

		private:
			void HandlePacket(Packets::S_ChunkCreate& chunkCreate);
			void HandlePacket(Packets::S_ChunkDestroy& chunkDestroy);
			void HandlePacket(Packets::S_ChunkReset& chunkReset);
			void HandlePacket(Packets::S_ChunkUpdate& chunkUpdate);
			void HandlePacket(Packets::S_DebugDrawLineList& debugDrawLineList);
			void HandlePacket(Packets::S_EntitiesCreation& entitiesCreation);
			void HandlePacket(Packets::S_EntitiesDelete& entitiesDelete);
			void HandlePacket(Packets::S_EntitiesStateUpdate& stateUpdate);
			void HandlePacket(Packets::S_EntityEnvironmentUpdate& environmentUpdate);
			void HandlePacket(Packets::S_EntityProcedureCall& procedureCall);
			void HandlePacket(Packets::S_EntityPropertiesUpdate& propertyUpdate);
			void HandlePacket(Packets::S_EnvironmentCreate& envCreate);
			void HandlePacket(Packets::S_EnvironmentDestroy& envDestroy);
			void HandlePacket(Packets::S_EnvironmentsUpdateOwner& envOwnerUpdate);
			void HandlePacket(Packets::S_GameData& gameData);
			void HandlePacket(Packets::S_PilotShip& pilotShip);
			void HandlePacket(Packets::S_PlanetEnvironmentRotation& planetEnvironmentRotation);
			void HandleEntityCreation(Packets::Helper::EntityData& entityData);
			void SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData& entityData);
			void SetupRootEntities();
			void SetupPlayerModel();

			struct DebugDrawLines
			{
				std::size_t environmentId;
				std::vector<Nz::Vector3f> vertices;
				Nz::Color color;
				Nz::Quaternionf rotation;
				Nz::Time duration;
			};

			struct EnvironmentData
			{
				Nz::Bitset<Nz::UInt64> entities;
				entt::handle rootEntity;
				GravityController* gravityController;
				Nz::DegreeAnglef rotation = Nz::DegreeAnglef::Zero();
				Nz::Vector3f rotationAxis = Nz::Vector3f::UnitY();
				bool isRoot = false;
			};

			struct EntityData
			{
				Packets::Helper::EnvironmentId environmentIndex;
				entt::handle entity;
			};

			NazaraSlot(ClientSessionHandler, OnChunkCreate, m_onChunkCreate);
			NazaraSlot(ClientSessionHandler, OnChunkDestroy, m_onChunkDestroy);
			NazaraSlot(ClientSessionHandler, OnChunkReset, m_onChunkReset);
			NazaraSlot(ClientSessionHandler, OnChunkUpdate, m_onChunkUpdate);
			NazaraSlot(ClientSessionHandler, OnDebugDrawLineList, m_onDebugDrawLineList);
			NazaraSlot(ClientSessionHandler, OnEntitiesCreation, m_onEntitiesCreation);
			NazaraSlot(ClientSessionHandler, OnEntitiesDelete, m_onEntitiesDelete);
			NazaraSlot(ClientSessionHandler, OnEntitiesStateUpdate, m_onEntitiesStateUpdate);
			NazaraSlot(ClientSessionHandler, OnEntityEnvironmentUpdate, m_onEntityEnvironmentUpdate);
			NazaraSlot(ClientSessionHandler, OnEntityProcedureCall, m_onEntityProcedureCall);
			NazaraSlot(ClientSessionHandler, OnEntityPropertiesUpdate, m_onEntityPropertiesUpdate);
			NazaraSlot(ClientSessionHandler, OnEnvironmentCreate, m_onEnvironmentCreate);
			NazaraSlot(ClientSessionHandler, OnEnvironmentDestroy, m_onEnvironmentDestroy);
			NazaraSlot(ClientSessionHandler, OnEnvironmentsUpdateOwner, m_onEnvironmentsUpdateOwner);
			NazaraSlot(ClientSessionHandler, OnGameData, m_onGameData);
			NazaraSlot(ClientSessionHandler, OnPilotShip, m_onPilotShip);
			NazaraSlot(ClientSessionHandler, OnPlanetEnvironmentRotation, m_onPlanetEnvironmentRotation);

			entt::handle m_cameraEntity;
			entt::handle m_playerControlledEntity;
			entt::handle m_skyboxEntity;
			entt::handle m_sunLightEntity;
			std::array<float, 3> m_csmSplitFactors;
			std::shared_ptr<Nz::Collider3D> m_playerCollider;
			std::shared_ptr<Nz::MaterialInstance> m_skyboxMaterial;
			std::shared_ptr<Nz::Model> m_playerModel;
			std::shared_ptr<PlayerAnimationAssets> m_playerAnimAssets;
			std::vector<std::optional<EntityData>> m_entities; //< FIXME: Nz::SparseVector
			std::vector<std::optional<EnvironmentData>> m_environments; //< FIXME: Nz::SparseVector
			tsl::hopscotch_map<Nz::UInt64, DebugDrawLines> m_debugDrawLines;
			Nz::HybridVector<Packets::Helper::EnvironmentId, 3> m_rootEnvironments;
			ClientBlockLibrary& m_blockLibrary;
			ClientSessionHandler& m_sessionHandler;
			Nz::ApplicationBase& m_app;
			Nz::EnttWorld& m_world;
			Nz::DirectionalLight* m_directionalLight;
			Nz::ImGuiPlugin* m_imgui;
			Nz::Node m_rootTransform;
			Nz::UInt16 m_lastTickIndex;
			EntityRegistry m_entityRegistry;
			ScriptingContext m_scriptingContext;
	};
}

#include <ClientLib/ClientEnvironmentHandler.inl>

#endif // TSOM_CLIENTLIB_CLIENTENVIRONMENTHANDLER_HPP
