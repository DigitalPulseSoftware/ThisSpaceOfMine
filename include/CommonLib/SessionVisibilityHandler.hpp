// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SESSIONVISIBILITYHANDLER_HPP
#define TSOM_COMMONLIB_SESSIONVISIBILITYHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	class NetworkSession;

	class TSOM_COMMONLIB_API SessionVisibilityHandler
	{
		public:
			inline SessionVisibilityHandler(NetworkSession* networkSession);
			SessionVisibilityHandler(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler(SessionVisibilityHandler&&) = delete;
			~SessionVisibilityHandler() = default;

			void CreateEntity(entt::handle entity, bool isMoving);
			void DestroyEntity(entt::handle entity);

			void Dispatch();

			SessionVisibilityHandler& operator=(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler& operator=(SessionVisibilityHandler&&) = delete;

		private:
			static constexpr std::size_t FreeIdGrowRate = 512;

			struct HandlerHasher
			{
				inline std::size_t operator()(const entt::handle& handle) const;
			};

			tsl::hopscotch_map<entt::handle, Nz::UInt32, HandlerHasher> m_entityToNetworkId;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_createdEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_deletedEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_movingEntities;
			Nz::Bitset<Nz::UInt64> m_freeEntityIds;
			NetworkSession* m_networkSession;
	};
}

#include <CommonLib/SessionVisibilityHandler.inl>

#endif
