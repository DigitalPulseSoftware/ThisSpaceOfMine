// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Components/DistributionComponent.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	void DistributionComponent::Connect(entt::handle sourceEntity, entt::handle targetEntity, std::size_t sourceOutputPort, std::size_t targetInputPort)
	{
		auto* sourceEntityDistribution = sourceEntity.try_get<DistributionComponent>();
		if (!sourceEntityDistribution)
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity which has no distribution component (is the entity correctly initialized?)");
			return;
		}

		auto* targetEntityDistribution = targetEntity.try_get<DistributionComponent>();
		if (!targetEntityDistribution)
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect to entity {} which has no distribution component (is the entity correctly initialized?)");
			return;
		}

		return Connect(sourceEntity, *sourceEntityDistribution, targetEntity, *targetEntityDistribution, sourceOutputPort, targetInputPort);
	}

	void DistributionComponent::Connect(entt::handle sourceEntity, DistributionComponent& sourceEntityDistribution, entt::handle targetEntity, DistributionComponent& targetEntityDistribution, std::size_t sourceOutputPort, std::size_t targetInputPort)
	{
		if (sourceOutputPort >= sourceEntityDistribution.GetOutputCount())
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity on output port {} but it only got {} port(s) (is the entity correctly initialized?)", sourceOutputPort, sourceEntityDistribution.GetOutputCount());
			return;
		}

		if (targetInputPort >= targetEntityDistribution.GetInputCount())
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect to entity on input port {} but it only got {} port(s) (is the entity correctly initialized?)", targetInputPort, targetEntityDistribution.GetInputCount());
			return;
		}

		if (sourceEntityDistribution.GetOutputType(sourceOutputPort) != targetEntityDistribution.GetInputType(targetInputPort))
		{
			// TODO: Log entities ID and type
			spdlog::error("Tried to connect entity on output port {} to entity on input port {} which has incompatible type", sourceOutputPort, targetInputPort);
			return;
		}

		// Disconnect already connected entity if any
		if (entt::handle connectedEntity = sourceEntityDistribution.GetOutputConnectedEntity(sourceOutputPort))
		{
			auto& connectedDistribution = connectedEntity.get<DistributionComponent>();
			connectedDistribution.DisconnectInput(sourceEntityDistribution.GetOutputConnectedPort(sourceOutputPort));
		}

		if (entt::handle connectedEntity = targetEntityDistribution.GetInputConnectedEntity(targetInputPort))
		{
			auto& connectedDistribution = connectedEntity.get<DistributionComponent>();
			connectedDistribution.DisconnectOutput(targetEntityDistribution.GetInputConnectedPort(targetInputPort));
		}

		// Register the connection
		sourceEntityDistribution.ConnectOutput(sourceOutputPort, targetEntity, targetInputPort);
		targetEntityDistribution.ConnectInput(targetInputPort, sourceEntity, sourceOutputPort);
	}

	void DistributionComponent::DisconnectInput(entt::handle entity, std::size_t inputPort)
	{
		auto* entityDistribution = entity.try_get<DistributionComponent>();
		if (!entityDistribution)
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity which has no distribution component (is the entity correctly initialized?)");
			return;
		}

		if (inputPort >= entityDistribution->GetInputCount())
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity on output port {} but it only got {} port(s) (is the entity correctly initialized?)", inputPort, entityDistribution->GetInputCount());
			return;
		}

		if (entt::handle connectedEntity = entityDistribution->GetInputConnectedEntity(inputPort))
		{
			auto& connectedDistribution = connectedEntity.get<DistributionComponent>();
			connectedDistribution.DisconnectOutput(entityDistribution->GetInputConnectedPort(inputPort));
		}

		entityDistribution->DisconnectInput(inputPort);
	}

	void DistributionComponent::DisconnectOutput(entt::handle entity, std::size_t outputPort)
	{
		auto* entityDistribution = entity.try_get<DistributionComponent>();
		if (!entityDistribution)
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity which has no distribution component (is the entity correctly initialized?)");
			return;
		}

		if (outputPort >= entityDistribution->GetOutputCount())
		{
			// TODO: Log entities ID
			spdlog::error("Tried to connect entity on output port {} but it only got {} port(s) (is the entity correctly initialized?)", outputPort, entityDistribution->GetOutputCount());
			return;
		}

		if (entt::handle connectedEntity = entityDistribution->GetOutputConnectedEntity(outputPort))
		{
			auto& connectedDistribution = connectedEntity.get<DistributionComponent>();
			connectedDistribution.DisconnectInput(entityDistribution->GetOutputConnectedPort(outputPort));
		}

		entityDistribution->DisconnectOutput(outputPort);
	}

	void DistributionComponent::ClearDistributedValues()
	{
		for (auto&& [type, distributedValue] : m_distributedValues.iter_kv())
		{
			std::visit([](auto&& quantity) { quantity.Clear(); }, distributedValue);
		}
	}

	void DistributionComponent::IncrementDistributedValue(DistributionType type, const DistributionQuantity& value)
	{
		NazaraAssert(value.index() == m_distributedValues[type].index());
		std::visit([&](auto&& quantity)
		{
			using T = std::decay_t<decltype(quantity)>;
			quantity += std::get<T>(value);
		},
		m_distributedValues[type]);
	}
}
