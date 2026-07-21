// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLAYER_HPP
#define TSOM_SERVERLIB_SERVERPLAYER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/EntityReference.hpp>
#include <CommonLib/PlayerIndex.hpp>
#include <CommonLib/PlayerPermission.hpp>
#include <ServerLib/SessionVisibilityHandler.hpp>
#include <Nazara/Core/HandledObject.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <Nazara/Core/Uuid.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/PrivateImpl.hpp>
#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace tsom
{
	class CharacterController;
	class NetworkSession;
	class ServerEnvironment;
	class ServerPlayer;
	class ServerInstance;
	class ServerShipEnvironment;

	using ServerPlayerHandle = Nz::ObjectHandle<ServerPlayer>;

	class TSOM_SERVERLIB_API ServerPlayer : public Nz::HandledObject<ServerPlayer>
	{
		public:
			ServerPlayer(ServerInstance& instance, PlayerIndex playerIndex, NetworkSession* session, const std::optional<Nz::Uuid>& uuid, std::string nickname, PlayerPermissionFlags permissions);
			ServerPlayer(const ServerPlayer&) = delete;
			ServerPlayer(ServerPlayer&&) = delete;
			~ServerPlayer();

			void AddToEnvironment(ServerEnvironment* environment, entt::handle environmentOwner);

			void ClearEnvironments();

			void Destroy();

			void ExecuteConsoleCommand(std::string_view command);

			void ExitPiloting();

			inline const std::shared_ptr<CharacterController>& GetCharacterController();
			inline EntityReference& GetControlledEntityReference();
			inline const EntityReference& GetControlledEntityReference() const;
			ServerEnvironment* GetControlledEntityEnvironment();
			const ServerEnvironment* GetControlledEntityEnvironment() const;
			EntityReference GetControlledShipEntityReference() const;
			inline ServerEnvironment* GetRootEnvironment();
			inline const ServerEnvironment* GetRootEnvironment() const;
			inline const std::string& GetNickname() const;
			inline PlayerPermissionFlags GetPermissions() const;
			inline PlayerIndex GetPlayerIndex() const;
			inline ServerInstance& GetServerInstance();
			inline const ServerInstance& GetServerInstance() const;
			inline NetworkSession* GetSession();
			inline const NetworkSession* GetSession() const;
			inline SessionVisibilityHandler& GetVisibilityHandler();
			inline const SessionVisibilityHandler& GetVisibilityHandler() const;
			inline const std::optional<Nz::Uuid>& GetUuid() const;

			void GrabEntity(EntityReference entity, const Nz::Vector3f& grabOffset);

			inline bool HasGrabbedEntity() const;

			inline bool HasPermission(PlayerPermission permission);

			inline bool IsAuthenticated() const;

			inline bool IsInEnvironment(const ServerEnvironment* environment);

			void PilotShip(EntityReference shipEntity, EntityReference shipExteriorEntity, const Nz::Quaternionf& referenceRotation);

			void PushInputs(const PlayerInputs& inputs);

			void RemoveFromEnvironment(ServerEnvironment* environment);

			void Respawn(ServerEnvironment* environment, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			void SendChatMessage(std::string chatMessage);

			void SetOwnedShip(std::unique_ptr<ServerShipEnvironment>&& ship);

			void Tick();

			std::string ToString() const;

			void Ungrab();

			void UpdateNickname(std::string nickname);
			void UpdateRootEnvironment(ServerEnvironment* environment);

			ServerPlayer& operator=(const ServerPlayer&) = delete;
			ServerPlayer& operator=(ServerPlayer&&) = delete;

		private:
			struct Console;
			struct GrabConstraint;

			std::optional<Nz::Uuid> m_uuid;
			std::shared_ptr<CharacterController> m_controller;
			std::string m_nickname;
			std::unique_ptr<ServerShipEnvironment> m_ship;
			std::vector<ServerEnvironment*> m_registeredEnvironments;
			Nz::FixedVector<PlayerInputs, 10> m_inputBuffer;
			Nz::PrivateImpl<Console> m_console;
			Nz::PrivateImpl<GrabConstraint> m_grabConstraint;
			Nz::Time m_respawnTimer;
			Nz::UInt32 m_inputQueueAdvancement;
			NetworkSession* m_session;
			ServerEnvironment* m_rootEnvironment;
			SessionVisibilityHandler m_visibilityHandler;
			ServerInstance& m_serverInstance;
			EntityReference m_controlledEntity;
			PlayerIndex m_playerIndex;
			PlayerPermissionFlags m_permissions;
	};
}

#include <ServerLib/ServerPlayer.inl>

#endif // TSOM_SERVERLIB_SERVERPLAYER_HPP
