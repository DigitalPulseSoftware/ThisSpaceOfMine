// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ServerPlayer.hpp>
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

#include <CommonLib/Components/ServerPlayerControlledComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_SERVERPLAYERCONTROLLEDCOMPONENT_HPP
