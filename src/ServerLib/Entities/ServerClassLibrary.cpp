// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Entities/ServerClassLibrary.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/ShipController.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <ServerLib/Components/ShipExteriorComponent.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	void ServerClassLibrary::Register(EntityRegistry& registry)
	{
		auto playerCollider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);
		registry.RegisterClass(EntityClass("player", {},
		{
			.onActivate = [this](entt::handle entity)
			{
				auto& envSwitchComponent = entity.emplace<ServerEnvironmentSwitchComponent>();
				envSwitchComponent.handleEnvironmentSwitch = [](entt::handle previousEntity, entt::handle newEntity, const EnvironmentTransform& envTransform)
				{
					ServerEnvironment* previousEnvironment = ServerEnvironment::GetEnvironment(previousEntity);
					ServerEnvironment* newEnvironment = ServerEnvironment::GetEnvironment(newEntity);

					auto& oldPlayerControlled = previousEntity.get<ServerPlayerControlledComponent>();
					ServerPlayer* player = oldPlayerControlled.GetPlayer();

					auto& newNode = newEntity.get<Nz::NodeComponent>();

					auto& previousCharacter = previousEntity.get<Nz::PhysCharacter3DComponent>();
					auto [linearVel, angularVel] = previousCharacter.GetLinearAndAngularVelocity();
					linearVel = envTransform.Rotate(linearVel);
					angularVel = envTransform.Rotate(angularVel);

					Nz::Vector3f prevEnvironmentUp = -previousEnvironment->GetGravityController()->ComputeGravity(newNode.GetPosition()).direction;
					Nz::Quaternionf environmentRotationCorrection = Nz::Quaternionf::RotationBetween(prevEnvironmentUp, Nz::Vector3f::Up());

					auto& characterController = player->GetCharacterController();
					characterController->SetGravityController(newEnvironment->GetGravityController());
					characterController->RotateInstantaneously(envTransform.rotation);

					Nz::PhysCharacter3DComponent::Settings characterSettings;
					characterSettings.collider = previousCharacter.GetCollider();
					characterSettings.objectLayer = previousCharacter.GetObjectLayer();
					characterSettings.position = newNode.GetPosition();
					characterSettings.rotation = newNode.GetRotation();

					auto& characterComponent = newEntity.emplace<Nz::PhysCharacter3DComponent>(std::move(characterSettings));
					characterComponent.SetImpl(characterController);
					characterComponent.SetLinearAndAngularVelocity(linearVel, angularVel);
					characterComponent.SetUp(newNode.GetUp());
					characterComponent.DisableSleeping();

					// Force controller update to ensure new position will be sent
					characterController->UpdatePosition(characterComponent);

					newEntity.emplace<ServerPlayerControlledComponent>(player->CreateHandle());

					if (newEnvironment->IsRoot())
						player->UpdateRootEnvironment(newEnvironment);
				};
			},
			.onInit = [this, playerCollider](entt::handle entity)
			{
				auto& playerControlled = entity.get<ServerPlayerControlledComponent>();
				ServerPlayer* player = playerControlled.GetPlayer();
				NazaraAssert(player);

				Nz::PhysCharacter3DComponent::Settings characterSettings;
				characterSettings.collider = playerCollider;
				characterSettings.objectLayer = Constants::ObjectLayerPlayer;

				auto characterController = player->GetCharacterController();

				auto& characterComponent = entity.emplace<Nz::PhysCharacter3DComponent>(std::move(characterSettings));
				characterComponent.SetImpl(characterController);
				characterComponent.DisableSleeping();

				player->GetVisibilityHandler().UpdateControlledEntity(entity, characterController.get()); // TODO: Reset to nullptr when player entity is destroyed
			}
		},
		{}));

		registry.RegisterClass(EntityClass("ship_exterior", {},
		{
			.onActivate = [this](entt::handle entity)
			{
				auto& shipExterior = entity.get<ShipExteriorComponent>();
				ServerShipEnvironment* shipEnvironment = shipExterior.ownerShip;

				auto& envProxy = entity.emplace<EnvironmentProxyComponent>();
				envProxy.targetEnvironment = shipEnvironment;

				auto& shipEntry = entity.emplace<EnvironmentEnterTriggerComponent>();
				shipEntry.targetEnvironment = shipEnvironment;

				if (const auto& interiorAreaCollider = shipEnvironment->GetInteriorAreaCollider())
				{
					shipEntry.entryTrigger = interiorAreaCollider;
					shipEntry.aabb = interiorAreaCollider->GetBoundingBox();
					shipEntry.aabb.Translate(interiorAreaCollider->GetCenterOfMass());
				}
				else
					shipEntry.enabled = false;

				shipExterior.onInteriorColliderUpdated.Connect(shipEnvironment->OnInteriorColliderUpdated, [entity, shipEnvironment]()
				{
					auto& shipEntry = entity.get<EnvironmentEnterTriggerComponent>();
					if (const auto& interiorAreaCollider = shipEnvironment->GetInteriorAreaCollider())
					{
						shipEntry.enabled = true;
						shipEntry.entryTrigger = interiorAreaCollider;
						shipEntry.aabb = interiorAreaCollider->GetBoundingBox();
						shipEntry.aabb.Translate(interiorAreaCollider->GetCenterOfMass());
					}
					else
						shipEntry.enabled = false;
				});

				auto& envSwitchComponent = entity.emplace<ServerEnvironmentSwitchComponent>();
				envSwitchComponent.handleEnvironmentSwitch = [shipEnvironment](entt::handle previousEntity, entt::handle newEntity, const EnvironmentTransform& envTransform)
				{
					ServerEnvironment* previousEnvironment = ServerEnvironment::GetEnvironment(previousEntity);
					ServerEnvironment* newEnvironment = ServerEnvironment::GetEnvironment(newEntity);

					auto& oldRigidBody = previousEntity.get<Nz::RigidBody3DComponent>();

					auto [linearVel, angularVel] = oldRigidBody.GetLinearAndAngularVelocity();
					auto physicsSettings = std::get<Nz::RigidBody3D::DynamicSettings>(oldRigidBody.GetSettings());
					physicsSettings.linearVelocity = envTransform.Rotate(linearVel);
					physicsSettings.angularVelocity = envTransform.Rotate(angularVel);

					newEntity.emplace<Nz::RigidBody3DComponent>(physicsSettings);
					newEntity.emplace<ShipExteriorComponent>().ownerShip = shipEnvironment;

					// Change the root environment of players in the ship as well
					if (newEnvironment->IsRoot())
					{
						entt::registry& registry = shipEnvironment->GetWorld().GetRegistry();
						auto playerView = registry.view<ServerPlayerControlledComponent>();
						for (entt::entity entity : playerView)
						{
							ServerPlayer* player = playerView.get<ServerPlayerControlledComponent>(entity).GetPlayer();
							if (player)
								player->UpdateRootEnvironment(newEnvironment);
						}
					}
				};
			},
			.onInit = [this](entt::handle entity)
			{
				auto& shipExterior = entity.get<ShipExteriorComponent>();

				ServerShipEnvironment* shipEnvironment = shipExterior.ownerShip;

				Nz::RigidBody3D::DynamicSettings physSettings(shipEnvironment->GetShip().BuildHullCollider(), 100.f);
				physSettings.objectLayer = Constants::ObjectLayerDynamic;
				physSettings.linearDamping = 0.f;

				entity.emplace<Nz::RigidBody3DComponent>(physSettings);
			}
		},
		{
		}));
	}
}
