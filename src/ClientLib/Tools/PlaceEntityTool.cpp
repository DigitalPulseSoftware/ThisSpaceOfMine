// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/PlaceEntityTool.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Physics3D/PhysFilter3D.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <spdlog/spdlog.h>
#include <numeric>

#ifdef TSOM_DEV_TOOLS
#include <imgui.h>
#endif

namespace tsom
{
	void PlaceEntityTool::OnActivate()
	{
		if (m_selectedEntityClass.empty())
		{
			// First time: select which entity to spawn
			m_isSelectingEntities = true;
			m_isCursorUnlocked = true;
		}

		m_preview.emplace();
		m_preview->material = m_assetLibrary.GetMaterialInstance("preview_mat")->Clone();

		// Refresh on activation for script reloading
		RefreshEntityClasses();
	}

	void PlaceEntityTool::OnDeactivate()
	{
		if (m_preview)
		{
			if (m_preview->entity)
				m_preview->entity.destroy();

			m_preview.reset();
		}
	}

	void PlaceEntityTool::OnTrigger(TriggerType triggerType)
	{
		if (triggerType != TriggerType::Primary && triggerType != TriggerType::Tertiary)
			return;

		auto raycastHit = m_gameInterface.RaycastQuery();
		if (!raycastHit)
			return;

		auto* chunkComponent = raycastHit->hitEntity.try_get<ChunkComponent>();
		if (!chunkComponent)
			return;

		auto& chunkNetworkMap = chunkComponent->parentEntity.get<ChunkNetworkMapComponent>();
		auto& chunkRigidBody = raycastHit->hitEntity.get<Nz::RigidBody3DComponent>();
		auto& chunkNode = raycastHit->hitEntity.get<Nz::NodeComponent>();

		const Chunk& hitChunk = *chunkComponent->chunk;
		const ChunkContainer& chunkContainer = hitChunk.GetContainer();

		Nz::Vector3f localPos = chunkNode.ToLocalPosition(raycastHit->hitPos);
		Nz::Vector3f localNormal = chunkNode.ToLocalDirection(raycastHit->hitNormal);

		auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), raycastHit->subShapeID);
		if (!hitCoordinates)
			return;

		if (triggerType == TriggerType::Primary)
		{
			NetworkSession* networkSession = m_gameInterface.GetNetworkSession();

			Packets::C_PlaceEntity placeEntity;
			placeEntity.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, &hitChunk);
			placeEntity.voxelLoc.x = hitCoordinates->blockIndices.x;
			placeEntity.voxelLoc.y = hitCoordinates->blockIndices.y;
			placeEntity.voxelLoc.z = hitCoordinates->blockIndices.z;
			placeEntity.topFace = hitCoordinates->direction;
			placeEntity.entityRotation = m_preview->rotationMultiplier;
			placeEntity.entityClass = networkSession->GetStringStore().CheckStringIndex(m_selectedEntityClass);

			networkSession->SendPacket(placeEntity);
		}
		else
		{
			m_isSelectingEntities = !m_isSelectingEntities;
			m_isCursorUnlocked = m_isSelectingEntities;
			m_gameInterface.UpdateMouseLock();
		}
	}

	void PlaceEntityTool::OnWheel(float delta)
	{
		if (delta < 0.f)
		{
			if (m_preview->rotationMultiplier == 0)
				m_preview->rotationMultiplier = 7;
			else
				m_preview->rotationMultiplier--;
		}
		else
		{
			if (m_preview->rotationMultiplier == 7)
				m_preview->rotationMultiplier = 0;
			else
				m_preview->rotationMultiplier++;
		}
	}

	void PlaceEntityTool::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* previewRaycast)
	{
		if (previewRaycast)
		{
			if (auto* chunkComponent = previewRaycast->hitEntity.try_get<ChunkComponent>())
			{
				auto& chunkRigidBody = previewRaycast->hitEntity.get<Nz::RigidBody3DComponent>();
				auto& chunkNode = previewRaycast->hitEntity.get<Nz::NodeComponent>();

				const Chunk& hitChunk = *chunkComponent->chunk;
				const ChunkContainer& chunkContainer = hitChunk.GetContainer();

				Nz::Vector3f localPos = chunkNode.ToLocalPosition(previewRaycast->hitPos);
				Nz::Vector3f localNormal = chunkNode.ToLocalDirection(previewRaycast->hitNormal);

				auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), previewRaycast->subShapeID);
				if (hitCoordinates)
				{
					NazaraAssert(m_preview);
					if (!m_preview->entity)
					{
						const EntityRegistry& entityRegistry = m_gameInterface.GetEntityRegistry();

						const auto& entityClass = entityRegistry.FindClass(m_selectedEntityClass);
						if (entityClass)
						{
							const Nz::ParameterList& metadata = entityClass->GetMetadata();
							if (auto result = metadata.GetStringViewParameter("spawnable_model"))
							{
								std::shared_ptr<Nz::Model> previewModel = m_assetLibrary.GetModel(*result);
								if (previewModel)
								{
									previewModel = previewModel->Clone();

									for (std::size_t i = 0; i < previewModel->GetMaterialCount(); ++i)
										previewModel->SetMaterial(i, m_preview->material);
								}
								else
									spdlog::warn("invalid model {} (no preview)", *result);

								m_preview->entity = m_gameInterface.GetWorld().CreateEntity();
								m_preview->entity.emplace<Nz::NodeComponent>();
								m_preview->entity.emplace<Nz::GraphicsComponent>(std::move(previewModel));

								m_preview->collider.x = static_cast<float>(metadata.GetDoubleParameter("spawnable_collider.x").GetValueOr(1.0));
								m_preview->collider.y = static_cast<float>(metadata.GetDoubleParameter("spawnable_collider.y").GetValueOr(1.0));
								m_preview->collider.z = static_cast<float>(metadata.GetDoubleParameter("spawnable_collider.z").GetValueOr(1.0));

								m_preview->offset.x = static_cast<float>(metadata.GetDoubleParameter("spawnable_offset.x").GetValueOr(0.0));
								m_preview->offset.y = static_cast<float>(metadata.GetDoubleParameter("spawnable_offset.y").GetValueOr(0.0));
								m_preview->offset.z = static_cast<float>(metadata.GetDoubleParameter("spawnable_offset.z").GetValueOr(0.0));

								m_preview->rotationAxis.x = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotationaxis.x").GetValueOr(0.0));
								m_preview->rotationAxis.y = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotationaxis.y").GetValueOr(1.0));
								m_preview->rotationAxis.z = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotationaxis.z").GetValueOr(0.0));

								m_preview->keepUpright = metadata.GetBooleanParameter("spawnable_keepupright").GetValueOr(false);
							}
						}
					}

					if (m_preview->entity)
					{
						auto cornerPos = hitChunk.ComputeBlockCorners(hitCoordinates->blockIndices);
						auto& corners = s_faceCorners[hitCoordinates->direction];
						std::array<Nz::Vector3f, 4> cornerGlobalPos;
						for (std::size_t i = 0; i < 4; ++i)
							cornerGlobalPos[i] = chunkNode.ToGlobalPosition(cornerPos[corners[i]]);
						Nz::Vector3f faceCenter = std::accumulate(cornerGlobalPos.begin(), cornerGlobalPos.end(), Nz::Vector3f::Zero()) / corners.size();

						auto& previewNode = m_preview->entity.get<Nz::NodeComponent>();

						Nz::Vector3f entityPos = faceCenter;
						Nz::Quaternionf localRotation = Nz::Quaternionf(Nz::DegreeAnglef(45.f) * m_preview->rotationMultiplier, m_preview->rotationAxis);
						Nz::Quaternionf surfaceRotation = Nz::Quaternionf::Identity();
						Nz::Vector3f normal = s_dirNormals[hitCoordinates->direction];

						if (m_preview->keepUpright)
							entityPos += normal * m_preview->collider * 0.5f;
						else
						{
							entityPos += normal * m_preview->collider.y * 0.5f;

							Nz::Quaternionf correctionRotation = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Forward());
							surfaceRotation = Nz::Quaternionf::CombineRotations(correctionRotation, Nz::Quaternionf::RotationBetween(Nz::Vector3f::Forward(), normal));
						}

						Nz::Quaternionf entityRotation = chunkNode.ToGlobalRotation(Nz::Quaternionf::CombineRotations(localRotation, surfaceRotation));
						previewNode.SetTransform(entityPos - entityRotation * m_preview->offset, entityRotation);

						Nz::Matrix4f physicsMatrix = Nz::Matrix4f::Transform(entityPos, entityRotation);

						struct IgnoreTrigger : Nz::PhysObjectLayerFilter3D
						{
							bool ShouldCollide(Nz::PhysObjectLayer3D objectLayer) const override
							{
								return objectLayer != Constants::ObjectLayerStatic;
							}
						};

						IgnoreTrigger physObjectLayerFilter;

						Nz::OrientedBoxf obb(Nz::Boxf(m_preview->collider * -0.5f, m_preview->collider));
						obb.Update(physicsMatrix);

						m_gameInterface.GetDebugDrawer()->DrawBoxCorners(obb.GetCorners(), Nz::Color::Cyan());

						Nz::BoxCollider3D collider(m_preview->collider * 0.9f);

						auto& physSystem = m_gameInterface.GetWorld().GetSystem<Nz::Physics3DSystem>();
						bool doesCollide = physSystem.CollisionQuery(collider, physicsMatrix, [](const Nz::Physics3DSystem::ShapeCollisionInfo& hitInfo) -> std::optional<float>
						{
							return hitInfo.penetrationDepth;
						});

						m_preview->material->SetValueProperty("BaseColor", doesCollide ? Nz::Color(0.8f, 0.2f, 0.2f, 0.5f) : Nz::Color(1.f, 1.f, 1.f, 0.5f));
					}
				}
			}
		}

#ifdef TSOM_DEV_TOOLS
		if (m_isSelectingEntities)
		{
			if (ImGui::Begin("Entity selection", &m_isSelectingEntities))
			{
				ImGui::Text("Select the entity you wish to use");

				for (std::string_view className : m_spawnableClasses)
				{
					if (ImGui::Button(className.data()))
					{
						m_selectedEntityClass = className;
						m_isSelectingEntities = false;
						m_isCursorUnlocked = false;

						m_gameInterface.UpdateMouseLock();

						// force re-creation of preview entity
						if (m_preview->entity)
							m_preview->entity.destroy();
					}
				}
			}

			ImGui::End();
		}
#endif
	}

	void PlaceEntityTool::RefreshEntityClasses()
	{
		m_spawnableClasses.clear();

		const EntityRegistry& entityRegistry = m_gameInterface.GetEntityRegistry();
		entityRegistry.ForEachClass([&](std::string_view className, const EntityClass& entityClass)
		{
			const Nz::ParameterList& metadata = entityClass.GetMetadata();
			if (auto result = metadata.GetBooleanParameter("spawnable"); !result || !*result)
				return; //< not spawnable

			m_spawnableClasses.push_back(className);
		});

		std::sort(m_spawnableClasses.begin(), m_spawnableClasses.end());
	}
}
