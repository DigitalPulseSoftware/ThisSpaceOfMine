// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_SERVERENVIRONMENTSWITCHCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_SERVERENVIRONMENTSWITCHCOMPONENT_HPP

#include <CommonLib/EnvironmentTransform.hpp>
#include <NazaraUtils/Signal.hpp>
#include <entt/fwd.hpp>
#include <functional>

namespace tsom
{
	class ServerEnvironment;

	struct ServerEnvironmentSwitchComponent
	{
		void Switch(entt::handle entity, ServerEnvironment* previousEnvironment, ServerEnvironment* newEnvironment, const EnvironmentTransform& relativeTransform);

		std::function<void(entt::handle previousEntity, entt::handle newEntity, const EnvironmentTransform& /*relativeTransform*/)> handleEnvironmentSwitch;

		NazaraSignal(OnEntitySwitchedEnvironment, entt::handle /*previousEntity*/, entt::handle /*newEntity*/, ServerEnvironment* /*newEnvironment*/, const EnvironmentTransform& /*relativeTransform*/);
	};
}

#endif // TSOM_SERVERLIB_COMPONENTS_SERVERENVIRONMENTSWITCHCOMPONENT_HPP
