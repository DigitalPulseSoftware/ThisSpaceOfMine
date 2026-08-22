// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/SpotLight.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	struct FlashlightHolder
	{
		entt::handle lightEntity;
	};

	void ClientEntityClassLibrary::OnPlayerActivate(entt::handle entity)
	{
		auto& entityInstance = entity.get<ClassInstanceComponent>();

		Nz::UInt32 flashlightProperty = entityInstance.FindPropertyIndex("flashlight");
		NazaraAssert(flashlightProperty != entityInstance.InvalidIndex);

		using BoolProperty = EntityPropertySingleValue<EntityPropertyType::Bool>;

		if (std::get<BoolProperty>(entityInstance.GetProperty(flashlightProperty)).value)
			OnPlayerFlashLightEnabled(entity, true);

		entityInstance.OnPropertyUpdate.Connect([this, flashlightProperty, entity](ClassInstanceComponent* /*classInstance*/, Nz::UInt32 propertyIndex, const EntityProperty& newValue)
		{
			if (propertyIndex != flashlightProperty)
				return;

			OnPlayerFlashLightEnabled(entity, std::get<BoolProperty>(newValue).value);
		});
	}

	void ClientEntityClassLibrary::OnPlayerFlashLightEnabled(entt::handle entity, bool isEnabled)
	{
		if (isEnabled)
		{
			FlashlightHolder& flashLightHolder = entity.get_or_emplace<FlashlightHolder>();
			if (flashLightHolder.lightEntity)
			{
				auto& lightComponent = flashLightHolder.lightEntity.get<Nz::LightComponent>();
				lightComponent.Show();
			}
			else
			{
				flashLightHolder.lightEntity = entt::handle(*entity.registry(), entity.registry()->create());
				entity.get_or_emplace<EntityOwnerComponent>().Register(flashLightHolder.lightEntity);

				auto& lightNode = flashLightHolder.lightEntity.emplace<Nz::NodeComponent>();
				lightNode.SetParent(entity);
				//lightNode.SetPosition(Nz::Vector3f::Forward() * 0.1f);

				auto& lightComponent = flashLightHolder.lightEntity.emplace<Nz::LightComponent>();
				auto& spotLight = lightComponent.AddLight<Nz::SpotLight>();
				spotLight.UpdateEnergy(2.0f);
			}
		}
		else if (FlashlightHolder* flashLightHolder = entity.try_get<FlashlightHolder>())
		{
			auto& light = flashLightHolder->lightEntity.get<Nz::LightComponent>();
			light.Hide();
		}
	}

	void ClientEntityClassLibrary::OnPlayerRpc_Death(entt::handle entity)
	{
	}
}
