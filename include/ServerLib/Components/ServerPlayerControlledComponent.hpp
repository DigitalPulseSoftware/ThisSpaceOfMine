// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace tsom
{
	class ServerPlayer;

	class ServerPlayerControlledComponent
	{
		public:
			inline ServerPlayerControlledComponent(ServerPlayerHandle serverPlayer);
			ServerPlayerControlledComponent(const ServerPlayerControlledComponent&) = delete;
			ServerPlayerControlledComponent(ServerPlayerControlledComponent&&) = default;
			~ServerPlayerControlledComponent() = default;

			inline ServerPlayer* GetPlayer();

			ServerPlayerControlledComponent& operator=(const ServerPlayerControlledComponent&) = delete;
			ServerPlayerControlledComponent& operator=(ServerPlayerControlledComponent&&) = default;

		private:
			ServerPlayerHandle m_controllingPlayer;
	};
}

#include <ServerLib/Components/ServerPlayerControlledComponent.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP
