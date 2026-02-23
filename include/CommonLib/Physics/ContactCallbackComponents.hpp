// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
	struct PhysContact3D;
	struct PhysContactResponse3D;
}

namespace tsom::Physics
{
	struct ContactAddedCallbackComponent
	{
		std::function<void(entt::handle /*entity*/, const Nz::PhysBody3D* /*body*/, entt::handle /*otherEntity*/, const Nz::PhysBody3D* /*otherBody*/, const Nz::PhysContact3D& /*physContact*/, Nz::PhysContactResponse3D& /*physContactResponse*/)> callback;
	};

	struct ContactPersistedCallbackComponent
	{
		std::function<void(entt::handle /*entity*/, const Nz::PhysBody3D* /*body*/, entt::handle /*otherEntity*/, const Nz::PhysBody3D* /*otherBody*/, const Nz::PhysContact3D& /*physContact*/, Nz::PhysContactResponse3D& /*physContactResponse*/)> callback;
	};

	struct ContactRemovedCallbackComponent
	{
		std::function<void(entt::handle /*entity1*/, Nz::UInt32 /*body1Index*/, const Nz::PhysBody3D* /*body1*/, Nz::UInt32 /*subShapeID1*/, entt::handle /*entity2*/, Nz::UInt32 /*body2Index*/, const Nz::PhysBody3D* /*body2*/, Nz::UInt32 /*subShapeID2*/)> callback;
	};
}

#endif // TSOM_COMMONLIB_PHYSICS_CONTACTCALLBACKCOMPONENTS_HPP
