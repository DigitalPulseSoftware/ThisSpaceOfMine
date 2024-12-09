// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <CommonLib/EntityReference.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ServerEnvironmentSwitchComponent::Switch(entt::handle oldEntity, ServerEnvironment* previousEnvironment, ServerEnvironment* newEnvironment, const EnvironmentTransform& relativeTransform)
	{
		auto& previousNode = oldEntity.get<Nz::NodeComponent>();
		auto& previousClassInstance = oldEntity.get<ClassInstanceComponent>();

		Nz::Vector3f entityPosition = previousNode.GetPosition();
		Nz::Quaternionf entityRotation = previousNode.GetRotation();

		entt::handle newEntity = newEnvironment->CreateEntity();
		newEntity.emplace<Nz::NodeComponent>(relativeTransform.Translate(entityPosition), relativeTransform.Rotate(entityRotation));
		newEntity.emplace<ClassInstanceComponent>(previousClassInstance.GetClass());

		if (NetworkedComponent* networkComponent = oldEntity.try_get<NetworkedComponent>())
		{
			auto& networkedSystem = previousEnvironment->GetWorld().GetSystem<NetworkedEntitiesSystem>();
			networkedSystem.ForgetEntity(oldEntity);

			newEntity.emplace<NetworkedComponent>(false);

			previousEnvironment->ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				auto& visibilityHandler = serverPlayer.GetVisibilityHandler();
				visibilityHandler.UpdateEntityEnvironment(*newEnvironment, oldEntity, newEntity);
			});
		}

		if (EntityReference::HandleOwner* handleOwner = oldEntity.try_get<EntityReference::HandleOwner>())
		{
			auto& newHandleOwner = newEntity.emplace<EntityReference::HandleOwner>();
			newHandleOwner.handleData = std::move(handleOwner->handleData);
			newHandleOwner.handleData->entity = newEntity;
		}

		handleEnvironmentSwitch(oldEntity, newEntity, relativeTransform);
		newEntity.get<ClassInstanceComponent>().GetClass()->ActivateEntity(newEntity);

		OnEntitySwitchedEnvironment(oldEntity, newEntity, newEnvironment, relativeTransform);
		oldEntity.destroy();
	}
}
