// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_TICKCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_TICKCOMPONENT_HPP

#include <Nazara/Core/Time.hpp>
#include <entt/fwd.hpp>
#include <functional>

namespace tsom
{
	struct TickComponent
	{
		std::function<void(entt::handle)> onTick;
		Nz::Time timeBeforeTick = Nz::Time::Zero(); // 0 = tick each time
		Nz::Time tickRate = Nz::Time::Zero();
	};
}

#endif // TSOM_COMMONLIB_COMPONENTS_TICKCOMPONENT_HPP
