// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/DistributionSystem.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>

namespace tsom
{
	DistributionSystem::DistributionSystem(entt::registry& registry) :
	m_distributionConstructObserver(registry, entt::collector.group<DistributionComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_registry(registry)
	{
		m_distributionDestroyConnection = registry.on_destroy<DistributionComponent>().connect<&DistributionSystem::OnDistributionDestroy>(this);
	}

	DistributionSystem::~DistributionSystem()
	{
		m_distributionConstructObserver.disconnect();
	}

	void DistributionSystem::OnDistributionDestroy(entt::entity entity)
	{
		m_distributionEntities.erase(entity);
		m_producers.erase(entity);
	}

	void DistributionSystem::HandleDistributionEntity(entt::entity entity, DistributionComponent& distribution)
	{
		bool isProducer = false;
		for (std::size_t outputIndex = 0; outputIndex < distribution.GetOutputCount(); ++outputIndex)
		{
			if (distribution.IsOutputConnected(outputIndex))
			{
				isProducer = true;
				break;
			}
		}

		if (isProducer)
			m_producers.insert(entity);
		else
			m_producers.erase(entity);
	}

	void DistributionSystem::Update(Nz::Time elapsedTime)
	{
		m_distributionConstructObserver.each([this](entt::entity entity)
		{
			DistributionComponent& distributionComponent = m_registry.get<DistributionComponent>(entity);

			auto& entityData = m_distributionEntities[entity];
			entityData.onInputOutputChangedSlot.Connect(distributionComponent.OnInputOutputChanged, [this, entity](DistributionComponent* distributionComponent)
			{
				HandleDistributionEntity(entity, *distributionComponent);
			});

			HandleDistributionEntity(entity, distributionComponent);
		});

		for (auto [distributionEntity, entityDistribution]: m_registry.view<DistributionComponent>().each())
		{
			NazaraUnused(distributionEntity);
			entityDistribution.ClearDistributedValues();
		}

		for (entt::entity producerEntity : m_producers)
		{
			DistributionComponent& producerDistribution = m_registry.get<DistributionComponent>(producerEntity);
			for (std::size_t outputIndex = 0; outputIndex < producerDistribution.GetOutputCount(); ++outputIndex)
			{
				entt::handle connectedEntity = producerDistribution.GetOutputConnectedEntity(outputIndex);
				if (connectedEntity)
				{
					DistributionType type = producerDistribution.GetOutputType(outputIndex);
					Nz::UInt32 producedValue = producerDistribution.GetProductionValue(outputIndex);
					std::size_t connectedPort = producerDistribution.GetOutputConnectedPort(outputIndex);

					DistributionComponent& consumptionDistribution = m_registry.get<DistributionComponent>(connectedEntity);
					Nz::UInt32 consumedValue = consumptionDistribution.GetConsumptionValue(connectedPort);
					consumptionDistribution.IncrementDistributedValue(type, std::min(producedValue, consumedValue));
				}
			}
		}
	}
}
