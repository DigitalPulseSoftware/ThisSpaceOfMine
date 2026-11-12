// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PHYSICS_QUERYUTILS_HPP
#define TSOM_COMMONLIB_PHYSICS_QUERYUTILS_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Math/Matrix4.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <entt/entt.hpp>
#include <optional>

namespace Nz
{
	class Collider3D;
	class PhysBody3D;
	class Physics3DSystem;
	class PhysBodyFilter3D;
	class PhysBroadphaseLayerFilter3D;
	class PhysObjectLayerFilter3D;
}

namespace tsom
{
	class Chunk;
}

namespace tsom::Physics
{
	struct ShapeCollisionInfo
	{
		entt::handle hitEntity;
		const Nz::Collider3D* hitCollider = nullptr;
		Nz::PhysBody3D* hitBody = nullptr;
		Nz::Vector3f collisionPosition1;
		Nz::Vector3f collisionPosition2;
		Nz::Vector3f penetrationAxis;
		Nz::UInt32 subShapeID1;
		Nz::UInt32 subShapeID2;
		float penetrationDepth;

		const Chunk* hitChunk = nullptr;
		entt::handle hitChunkEntity; //< only valid if hitChunk != nullptr
	};

	struct RaycastHit
	{
		entt::handle hitEntity;
		const Nz::Collider3D* hitCollider = nullptr;
		Nz::PhysBody3D* hitBody = nullptr;
		Nz::Vector3f hitPosition;
		Nz::Vector3f hitNormal;
		Nz::UInt32 subShapeID;
		float fraction;

		const Chunk* hitChunk = nullptr;
		entt::handle hitChunkEntity; //< only valid if hitChunk != nullptr
	};

	TSOM_COMMONLIB_API bool CollisionQuery(Nz::Physics3DSystem& physicsSytem, const Nz::Collider3D& collider, const Nz::Matrix4f& colliderTransform, const Nz::FunctionRef<std::optional<float>(const ShapeCollisionInfo& hitInfo)>& callback, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter = nullptr, const Nz::PhysObjectLayerFilter3D* objectLayerFilter = nullptr, const Nz::PhysBodyFilter3D* bodyFilter = nullptr);
	TSOM_COMMONLIB_API bool Raycast(Nz::Physics3DSystem& physicsSytem, const Nz::Vector3f& start, const Nz::Vector3f& end, const Nz::FunctionRef<std::optional<float>(const RaycastHit& hitInfo)>& callback, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter = nullptr, const Nz::PhysObjectLayerFilter3D* objectLayerFilter = nullptr, const Nz::PhysBodyFilter3D* bodyFilter = nullptr);
	TSOM_COMMONLIB_API bool RaycastClosest(Nz::Physics3DSystem& physicsSytem, const Nz::Vector3f& start, const Nz::Vector3f& end, RaycastHit* hitInfo, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter = nullptr, const Nz::PhysObjectLayerFilter3D* objectLayerFilter = nullptr, const Nz::PhysBodyFilter3D* bodyFilter = nullptr);
}

#include <CommonLib/Physics/QueryUtils.inl>

#endif // TSOM_COMMONLIB_PHYSICS_QUERYUTILS_HPP
