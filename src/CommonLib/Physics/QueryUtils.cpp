// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Physics/QueryUtils.hpp>
#include <NazaraUtils/Assert.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ComplexCollisionComponent.hpp>

namespace tsom::Physics
{
	namespace NAZARA_ANONYMOUS_NAMESPACE
	{
		bool ProcessRaycastHit(const Nz::Vector3f& start, const Nz::Vector3f& rayDir, const Nz::Physics3DSystem::RaycastHit& hitInfo, RaycastHit* raycastHit)
		{
			Nz::Collider3D* hitCollider = hitInfo.hitBody->GetCollider().get();

			raycastHit->hitEntity = hitInfo.hitEntity;
			raycastHit->hitPosition = hitInfo.hitPosition;
			raycastHit->hitNormal = hitInfo.hitNormal;
			raycastHit->subShapeID = hitInfo.subShapeID;
			raycastHit->hitCollider = hitCollider;
			raycastHit->fraction = hitInfo.fraction;

			if (hitInfo.hitEntity)
			{
				if (ChunkComponent* hitChunkComponent = hitInfo.hitEntity.try_get<ChunkComponent>())
				{
					raycastHit->hitChunk = hitChunkComponent->chunk;
					raycastHit->hitChunkEntity = hitInfo.hitEntity;
					raycastHit->hitEntity = hitChunkComponent->parentEntity;
				}
				else
					raycastHit->hitEntity = hitInfo.hitEntity;
			}

			return true;
		}
	}

	bool CollisionQuery(Nz::Physics3DSystem& physicsSytem, const Nz::Collider3D& collider, const Nz::Matrix4f& colliderTransform, const Nz::FunctionRef<std::optional<float>(const ShapeCollisionInfo& hitInfo)>& callback, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter /*= nullptr*/, const Nz::PhysObjectLayerFilter3D* objectLayerFilter /*= nullptr*/, const Nz::PhysBodyFilter3D* bodyFilter /*= nullptr*/)
	{
		auto internalCallback = [&](const Nz::Physics3DSystem::ShapeCollisionInfo& hitInfo) -> std::optional<float>
		{
			Nz::Collider3D* hitCollider = hitInfo.hitBody->GetCollider().get();

			ShapeCollisionInfo collisionInfo;
			collisionInfo.hitBody = hitInfo.hitBody;
			collisionInfo.hitEntity = hitInfo.hitEntity;
			collisionInfo.collisionPosition1 = hitInfo.collisionPosition1;
			collisionInfo.collisionPosition2 = hitInfo.collisionPosition2;
			collisionInfo.penetrationAxis = hitInfo.penetrationAxis;
			collisionInfo.penetrationDepth = hitInfo.penetrationDepth;
			collisionInfo.subShapeID1 = hitInfo.subShapeID1;
			collisionInfo.subShapeID2 = hitInfo.subShapeID2;

			if (hitInfo.hitEntity)
			{
				if (ChunkComponent* hitChunkComponent = hitInfo.hitEntity.try_get<ChunkComponent>())
				{
					collisionInfo.hitChunk = hitChunkComponent->chunk;
					collisionInfo.hitChunkEntity = hitInfo.hitEntity;
					collisionInfo.hitEntity = hitChunkComponent->parentEntity;
				}
				else
					collisionInfo.hitEntity = hitInfo.hitEntity;
			}

			return callback(collisionInfo);
		};

		return physicsSytem.CollisionQuery(collider, colliderTransform, internalCallback, broadphaseFilter, objectLayerFilter, bodyFilter);
	}

	bool Raycast(Nz::Physics3DSystem& physicsSytem, const Nz::Vector3f& start, const Nz::Vector3f& end, const Nz::FunctionRef<std::optional<float>(const RaycastHit& hitInfo)>& callback, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter, const Nz::PhysObjectLayerFilter3D* objectLayerFilter, const Nz::PhysBodyFilter3D* bodyFilter)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		Nz::Vector3f rayDir = end - start; // dir * length
		auto internalCallback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
		{
			RaycastHit raycastHit;
			if (!ProcessRaycastHit(start, rayDir, hitInfo, &raycastHit))
				return {};

			return callback(raycastHit);
		};

		return physicsSytem.RaycastQuery(start, end, internalCallback, broadphaseFilter, objectLayerFilter, bodyFilter);
	}

	bool RaycastClosest(Nz::Physics3DSystem& physicsSytem, const Nz::Vector3f& start, const Nz::Vector3f& end, RaycastHit* hitInfo, const Nz::PhysBroadphaseLayerFilter3D* broadphaseFilter /*= nullptr*/, const Nz::PhysObjectLayerFilter3D* objectLayerFilter /*= nullptr*/, const Nz::PhysBodyFilter3D* bodyFilter /*= nullptr*/)
	{
		NAZARA_USE_ANONYMOUS_NAMESPACE

		Nz::Physics3DSystem::RaycastHit physHitInfo;

		Nz::Vector3f rayDir = end - start; // dir * length
		if (!physicsSytem.RaycastQueryClosest(start, end, &physHitInfo, broadphaseFilter, objectLayerFilter, bodyFilter))
			return false;

		if (!ProcessRaycastHit(start, rayDir, physHitInfo, hitInfo))
			return false;

		return true;
	}
}
