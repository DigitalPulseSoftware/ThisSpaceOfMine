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
#include <ServerLib/Components/ShipExteriorComponent.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	void ServerClassLibrary::Register(EntityRegistry& registry)
	{
		registry.RegisterClass(EntityClass("ship_exterior", {},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& shipExterior = entity.get<ShipExteriorComponent>();

				ServerShipEnvironment* shipEnvironment = shipExterior.ownerShip;
				ServerEnvironment* outsideEnvironment = entity.registry()->ctx().get<ServerEnvironment*>();

				Nz::RigidBody3D::DynamicSettings physSettings(shipEnvironment->GetShip().BuildHullCollider(), 100.f);
				physSettings.objectLayer = Constants::ObjectLayerDynamic;
				physSettings.linearDamping = 0.f;

				entity.emplace<Nz::RigidBody3DComponent>(physSettings);

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

				shipExterior.onInteriorColliderUpdated.Connect(shipEnvironment->OnInteriorColliderUpdated, [entity, shipEnvironment]()
				{
					auto& shipEntry = entity.get<EnvironmentEnterTriggerComponent>();
					if (const auto& interiorAreaCollider = shipEnvironment->GetInteriorAreaCollider())
					{
						shipEntry.entryTrigger = interiorAreaCollider;
						shipEntry.aabb = interiorAreaCollider->GetBoundingBox();
						shipEntry.aabb.Translate(interiorAreaCollider->GetCenterOfMass());
					}
				});

				auto& envSwitchComponent = entity.emplace<ServerEnvironmentSwitchComponent>();
				envSwitchComponent.handleEnvironmentSwitch = [shipEnvironment](entt::handle oldEntity, ServerEnvironment* newEnvironment, const EnvironmentTransform& envTransform)
				{
					ServerEnvironment* previousEnvironment = oldEntity.registry()->ctx().get<ServerEnvironment*>();

					auto& networkedSystem = previousEnvironment->GetWorld().GetSystem<NetworkedEntitiesSystem>();
					networkedSystem.ForgetEntity(oldEntity);

					Nz::NodeComponent& previousNode = oldEntity.get<Nz::NodeComponent>();
					Nz::Vector3f position = previousNode.GetPosition();
					Nz::Quaternionf rotation = previousNode.GetRotation();

					position = envTransform.Translate(position);
					rotation = envTransform.Rotate(rotation);

					auto& oldRigidBody = oldEntity.get<Nz::RigidBody3DComponent>();

					auto [linearVel, angularVel] = oldRigidBody.GetLinearAndAngularVelocity();
					linearVel = envTransform.Rotate(linearVel);
					angularVel = envTransform.Rotate(angularVel);

					entt::handle newEntity = shipEnvironment->LinkOutsideEnvironment(newEnvironment, EnvironmentTransform(position, rotation), false);
					newEntity.get<NetworkedComponent>().DontSendCreationSignal();

					auto& newRigidBody = newEntity.get<Nz::RigidBody3DComponent>();
					newRigidBody.SetLinearAndAngularVelocity(linearVel, angularVel);

					// Notify entity switched environment before destroying it
					// FIXME: Introduce a cross-environment entity handle to avoid needing this
					shipEnvironment->GetServerInstance().ForEachPlayer([&](ServerPlayer& player)
					{
						if (const auto& shipController = player.GetCharacterController()->GetShipController())
						{
							if (shipController->GetShipEntity() == oldEntity)
								shipController->UpdateShipEntity(newEntity);
						}

						auto& visibilityHandler = player.GetVisibilityHandler();
						visibilityHandler.UpdateEntityEnvironment(*newEnvironment, oldEntity, newEntity);
					});

					oldEntity.get<ServerEnvironmentSwitchComponent>().OnEntitySwitchedEnvironment(oldEntity, newEntity, newEnvironment, envTransform);
					oldEntity.destroy();
				};
			}
		},
		{
		}));
	}
}
