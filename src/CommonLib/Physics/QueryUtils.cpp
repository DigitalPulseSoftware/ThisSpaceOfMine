// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Physics/QueryUtils.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ComplexCollisionComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/Assert.hpp>

namespace tsom::Physics
{
	namespace NAZARA_ANONYMOUS_NAMESPACE
	{
		bool ProcessRaycastHit(const Nz::Vector3f& start, const Nz::Vector3f& rayDir, const Nz::Physics3DSystem::RaycastHit& hitInfo, RaycastHit* raycastHit)
		{
			Nz::Collider3D* hitCollider = hitInfo.hitBody->GetCollider().get();

			// Handle complex collisions
			if (hitInfo.hitEntity)
			{
				Nz::NodeComponent* entityNode = hitInfo.hitEntity.try_get<Nz::NodeComponent>();
				ComplexCollisionComponent* complexCollision = hitInfo.hitEntity.try_get<ComplexCollisionComponent>();
				ComplexChunkCollisionComponent* chunkComplexCollision = hitInfo.hitEntity.try_get<ComplexChunkCollisionComponent>();

				if (entityNode)
				{
					if (chunkComplexCollision)
					{
						NazaraAssert(hitCollider->GetType() == Nz::ColliderType3D::Compound);

						const Nz::CompoundCollider3D* hitCompoundCollider = Nz::SafeCast<const Nz::CompoundCollider3D*>(hitCollider);
						Nz::UInt32 colliderIndex = hitCompoundCollider->GetChildColliderIndexFromID(hitInfo.subShapeID);

						NazaraAssert(colliderIndex < chunkComplexCollision->chunkEntries.size());
						Nz::Collider3D* complexCollider = chunkComplexCollision->chunkEntries[colliderIndex].chunkCollider.get();
						const Nz::Vector3f& chunkOffset = chunkComplexCollision->chunkEntries[colliderIndex].chunkOffset;

						auto shapeCallback = [&](const Nz::Collider3D::RaycastHit& colliderHitInfo)
						{
							raycastHit->hitEntity = hitInfo.hitEntity;
							raycastHit->hitPosition = entityNode->ToGlobalPosition(colliderHitInfo.hitPosition + chunkOffset + complexCollider->GetCenterOfMass());
							raycastHit->hitNormal = entityNode->ToGlobalDirection(colliderHitInfo.hitNormal);
							raycastHit->subShapeID = colliderHitInfo.subShapeID;
							raycastHit->fraction = hitInfo.fraction;
							raycastHit->hitCollider = complexCollider;
							raycastHit->hitChunkEntity = chunkComplexCollision->chunkEntries[colliderIndex].chunkEntity;
							NazaraAssert(raycastHit->hitChunkEntity.all_of<ChunkComponent>());
							raycastHit->hitChunk = raycastHit->hitChunkEntity.get<ChunkComponent>().chunk;
						};

						// If there's a complex collider but we didn't hit it, ignore the whole entity
						Nz::Vector3f localOrigin = entityNode->ToLocalPosition(start) - chunkOffset - complexCollider->GetCenterOfMass();
						Nz::Vector3f localDestination = localOrigin + entityNode->ToLocalDirection(rayDir);
						return complexCollider->RaycastQuery(localOrigin, localDestination, shapeCallback);
					}
					else if (complexCollision)
					{
						Nz::Collider3D* complexCollider = complexCollision->complexCollisionCollider.get();

						auto shapeCallback = [&](const Nz::Collider3D::RaycastHit& colliderHitInfo)
						{
							raycastHit->hitPosition = entityNode->ToGlobalPosition(colliderHitInfo.hitPosition + complexCollider->GetCenterOfMass());
							raycastHit->hitNormal = entityNode->ToGlobalDirection(colliderHitInfo.hitNormal);
							raycastHit->subShapeID = colliderHitInfo.subShapeID;
							raycastHit->hitCollider = complexCollider;
							raycastHit->fraction = hitInfo.fraction;

							if (ChunkComponent* hitChunkComponent = hitInfo.hitEntity.try_get<ChunkComponent>())
							{
								raycastHit->hitChunk = hitChunkComponent->chunk;
								raycastHit->hitChunkEntity = hitInfo.hitEntity;
								raycastHit->hitEntity = hitChunkComponent->parentEntity;
							}
							else
								raycastHit->hitEntity = hitInfo.hitEntity;
						};

						// If there's a complex collider but we didn't hit it, ignore the whole entity
						Nz::Vector3f localOrigin = entityNode->ToLocalPosition(start) - complexCollider->GetCenterOfMass();
						Nz::Vector3f localDestination = localOrigin + entityNode->ToLocalDirection(rayDir);
						return complexCollider->RaycastQuery(localOrigin, localDestination, shapeCallback);
					}
				}
			}

			// No complex collisions
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

			// Handle complex collisions
			if (hitInfo.hitEntity)
			{
				Nz::NodeComponent* entityNode = hitInfo.hitEntity.try_get<Nz::NodeComponent>();
				ComplexCollisionComponent* complexCollision = hitInfo.hitEntity.try_get<ComplexCollisionComponent>();
				ComplexChunkCollisionComponent* chunkComplexCollision = hitInfo.hitEntity.try_get<ComplexChunkCollisionComponent>();

				if (entityNode)
				{
					if (chunkComplexCollision)
					{
						NazaraAssert(hitCollider->GetType() == Nz::ColliderType3D::Compound);

						const Nz::CompoundCollider3D* hitCompoundCollider = Nz::SafeCast<const Nz::CompoundCollider3D*>(hitCollider);
						Nz::UInt32 colliderIndex = hitCompoundCollider->GetChildColliderIndexFromID(hitInfo.subShapeID2);

						NazaraAssert(colliderIndex < chunkComplexCollision->chunkEntries.size());
						Nz::Collider3D* complexCollider = chunkComplexCollision->chunkEntries[colliderIndex].chunkCollider.get();
						const Nz::Vector3f& chunkOffset = chunkComplexCollision->chunkEntries[colliderIndex].chunkOffset;

						ShapeCollisionInfo collisionInfo;

						auto shapeCallback = [&](const Nz::Collider3D::ShapeCollisionInfo& shapeCollisionInfo) -> std::optional<float>
						{
							collisionInfo.collisionPosition1 = shapeCollisionInfo.collisionPosition1;
							collisionInfo.collisionPosition2 = shapeCollisionInfo.collisionPosition2;
							collisionInfo.penetrationAxis = shapeCollisionInfo.penetrationAxis;
							collisionInfo.penetrationDepth = shapeCollisionInfo.penetrationDepth;
							collisionInfo.hitCollider = complexCollider;
							collisionInfo.hitBody = hitInfo.hitBody;
							collisionInfo.subShapeID1 = shapeCollisionInfo.subShapeID1;
							collisionInfo.subShapeID2 = shapeCollisionInfo.subShapeID2;
							collisionInfo.hitChunkEntity = chunkComplexCollision->chunkEntries[colliderIndex].chunkEntity;
							NazaraAssert(collisionInfo.hitChunkEntity.all_of<ChunkComponent>());
							collisionInfo.hitChunk = collisionInfo.hitChunkEntity.get<ChunkComponent>().chunk;

							return shapeCollisionInfo.penetrationDepth;
						};

						// If there's a complex collider but we didn't hit it, ignore the whole entity
						Nz::Matrix4f complexColliderTransform = entityNode->GetTransformMatrix();
						complexColliderTransform.Translate(entityNode->GetGlobalRotation() * chunkOffset);

						if (!Nz::Collider3D::CollideShapes(collider, colliderTransform, *complexCollider, complexColliderTransform, shapeCallback))
							return {};

						return callback(collisionInfo);
					}
					else if (complexCollision)
					{
						Nz::Collider3D* complexCollider = complexCollision->complexCollisionCollider.get();

						ShapeCollisionInfo collisionInfo;

						auto shapeCallback = [&](const Nz::Collider3D::ShapeCollisionInfo& shapeCollisionInfo) -> std::optional<float>
						{
							collisionInfo.collisionPosition1 = shapeCollisionInfo.collisionPosition1;
							collisionInfo.collisionPosition2 = shapeCollisionInfo.collisionPosition2;
							collisionInfo.penetrationAxis = shapeCollisionInfo.penetrationAxis;
							collisionInfo.penetrationDepth = shapeCollisionInfo.penetrationDepth;
							collisionInfo.hitCollider = complexCollider;
							collisionInfo.hitBody = hitInfo.hitBody;
							collisionInfo.subShapeID1 = shapeCollisionInfo.subShapeID1;
							collisionInfo.subShapeID2 = shapeCollisionInfo.subShapeID2;

							if (ChunkComponent* hitChunkComponent = hitInfo.hitEntity.try_get<ChunkComponent>())
							{
								collisionInfo.hitChunk = hitChunkComponent->chunk;
								collisionInfo.hitChunkEntity = hitInfo.hitEntity;
								collisionInfo.hitEntity = hitChunkComponent->parentEntity;
							}
							else
								collisionInfo.hitEntity = hitInfo.hitEntity;

							return shapeCollisionInfo.penetrationDepth;
						};

						// If there's a complex collider but we didn't hit it, ignore the whole entity
						if (!Nz::Collider3D::CollideShapes(collider, colliderTransform, *complexCollider, entityNode->GetTransformMatrix(), shapeCallback))
							return {};

						return callback(collisionInfo);
					}
				}
			}

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
