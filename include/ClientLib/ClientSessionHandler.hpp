// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <entt/entt.hpp>
#include <unordered_map>

namespace Nz
{
	class EnttWorld;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientSessionHandler : public SessionHandler
	{
		public:
			ClientSessionHandler(NetworkSession* session, Nz::EnttWorld& world);
			~ClientSessionHandler() = default;

			void HandlePacket(Packets::AuthResponse&& authResponse);
			void HandlePacket(Packets::EntitiesCreation&& entitiesCreation);
			void HandlePacket(Packets::EntitiesDelete&& entitiesDelete);
			void HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate);

		private:
			std::unordered_map<Nz::UInt32, entt::handle> m_networkIdToEntity;
			Nz::EnttWorld& m_world;
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif
