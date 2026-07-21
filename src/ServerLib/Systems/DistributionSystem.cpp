// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/DistributionSystem.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>

namespace tsom
{
	DistributionSystem::DistributionSystem(entt::registry& registry) :
	m_distributionObserver(registry),
	m_tickAccumulator(Nz::Time::Zero()),
	m_registry(registry)
	{
		m_distributionObserver.OnEntityAdded.Connect([this](entt::entity entity)
		{
			DistributionComponent& distributionComponent = m_registry.get<DistributionComponent>(entity);

			auto& entityData = m_distributionObserver.Get(entity);
			entityData.onInputOutputChangedSlot.Connect(distributionComponent.OnInputOutputChanged, [this, entity](DistributionComponent* distributionComponent)
			{
				HandleDistributionEntity(entity, *distributionComponent);
			});

			HandleDistributionEntity(entity, distributionComponent);
		});

		m_distributionObserver.OnEntityRemove.Connect([this](entt::entity entity)
		{
			m_producers.remove(entity);
		});

		m_distributionObserver.SignalExisting();
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
			m_producers.emplace(entity);
		else
			m_producers.remove(entity);
	}

	void DistributionSystem::Update(Nz::Time elapsedTime)
	{
		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= Constants::DistributionTickInterval)
		{
			for (const auto& [distributionEntity, entityData] : m_distributionObserver.each())
			{
				NazaraUnused(entityData);

				DistributionComponent& entityDistribution = m_registry.get<DistributionComponent>(distributionEntity);
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

						DistributionComponent& consumptionDistribution = connectedEntity.get<DistributionComponent>();
						Nz::UInt32 consumedValue = consumptionDistribution.GetConsumptionValue(connectedPort);
						consumptionDistribution.IncrementDistributedValue(type, std::min(producedValue, consumedValue));
					}
				}
			}

			for (const auto& [distributionEntity, entityData] : m_distributionObserver.each())
			{
				NazaraUnused(entityData);
				DistributionComponent& entityDistribution = m_registry.get<DistributionComponent>(distributionEntity);
				entityDistribution.TriggerDistributionCallback(entt::handle(m_registry, distributionEntity));
			}

			m_tickAccumulator -= Constants::DistributionTickInterval;
		}
	}
}
