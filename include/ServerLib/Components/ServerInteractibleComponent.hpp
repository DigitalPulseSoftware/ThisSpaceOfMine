// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_SERVERINTERACTIBLECOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_SERVERINTERACTIBLECOMPONENT_HPP

#include <functional>

namespace tsom
{
	class ServerPlayer;

	struct ServerInteractibleComponent
	{
		std::function<void(entt::handle entity, ServerPlayer* player)> onInteraction;
		bool isEnabled;
	};
}

#endif // TSOM_SERVERLIB_COMPONENTS_SERVERINTERACTIBLECOMPONENT_HPP
