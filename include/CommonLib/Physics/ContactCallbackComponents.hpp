// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PHYSICS_CONTACTCALLBACKCOMPONENTS_HPP
#define TSOM_COMMONLIB_PHYSICS_CONTACTCALLBACKCOMPONENTS_HPP

#include <CommonLib/Export.hpp>
#include <entt/entt.hpp>
#include <functional>

namespace Nz
{
	class PhysBody3D;
}

namespace tsom::Physics
{
	struct ContactAddedCallbackComponent
	{
		std::function<void(entt::handle /*entity*/, const Nz::PhysBody3D* /*body*/, entt::handle /*otherEntity*/, const Nz::PhysBody3D* /*otherBody*/)> callback;
	};

	struct ContactRemovedCallbackComponent
	{
		std::function<void(entt::handle /*entity*/, const Nz::PhysBody3D* /*body*/, entt::handle /*otherEntity*/, const Nz::PhysBody3D* /*otherBody*/)> callback;
	};
}

#endif // TSOM_COMMONLIB_PHYSICS_CONTACTCALLBACKCOMPONENTS_HPP
