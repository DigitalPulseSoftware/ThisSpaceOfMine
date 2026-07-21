// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	inline DistributionComponent::DistributionComponent(std::span<DistributionType> inputs, std::span<DistributionType> outputs)
	{
		m_inputs.reserve(inputs.size());
		for (DistributionType distributionType : inputs)
		{
			auto& port = m_inputs.emplace_back();
			port.type = distributionType;
		}

		m_outputs.reserve(inputs.size());
		for (DistributionType distributionType : outputs)
		{
			auto& port = m_outputs.emplace_back();
			port.type = distributionType;
		}
	}

	inline void DistributionComponent::ConnectInput(std::size_t inputIndex, entt::handle entity, std::size_t outputIndex)
	{
		NazaraAssert(inputIndex < m_inputs.size());
		m_inputs[inputIndex].connectedEntity = entity;
		m_inputs[inputIndex].connectedPort = outputIndex;

		OnInputOutputChanged(this);
	}

	inline void DistributionComponent::ConnectOutput(std::size_t outputIndex, entt::handle entity, std::size_t inputIndex)
	{
		NazaraAssert(outputIndex < m_outputs.size());
		m_outputs[outputIndex].connectedEntity = entity;
		m_outputs[outputIndex].connectedPort = inputIndex;

		OnInputOutputChanged(this);
	}

	inline Nz::UInt32 DistributionComponent::GetConsumptionValue(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].consumptionValue;
	}

	inline Nz::UInt64 DistributionComponent::GetDistributedValue(DistributionType type) const
	{
		return m_distributedValues[type];
	}

	inline entt::handle DistributionComponent::GetInputConnectedEntity(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].connectedEntity;
	}

	inline std::size_t DistributionComponent::GetInputConnectedPort(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].connectedPort;
	}

	inline std::size_t DistributionComponent::GetInputCount() const
	{
		return m_inputs.size();
	}

	inline DistributionType DistributionComponent::GetInputType(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].type;
	}

	inline entt::handle DistributionComponent::GetOutputConnectedEntity(std::size_t outputIndex) const
	{
		NazaraAssert(outputIndex < m_outputs.size());
		return m_outputs[outputIndex].connectedEntity;
	}

	inline std::size_t DistributionComponent::GetOutputConnectedPort(std::size_t outputIndex) const
	{
		NazaraAssert(outputIndex < m_outputs.size());
		return m_outputs[outputIndex].connectedPort;
	}

	inline std::size_t DistributionComponent::GetOutputCount() const
	{
		return m_outputs.size();
	}

	inline DistributionType DistributionComponent::GetOutputType(std::size_t outputIndex) const
	{
		NazaraAssert(outputIndex < m_outputs.size());
		return m_outputs[outputIndex].type;
	}

	inline Nz::UInt32 DistributionComponent::GetProductionValue(std::size_t outputIndex) const
	{
		NazaraAssert(outputIndex < m_outputs.size());
		return m_outputs[outputIndex].productionValue;
	}

	inline bool DistributionComponent::IsInputConnected(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].connectedEntity.IsValid();
	}

	inline bool DistributionComponent::IsOutputConnected(std::size_t outputIndex) const
	{
		NazaraAssert(outputIndex < m_outputs.size());
		return m_outputs[outputIndex].connectedEntity.IsValid();
	}

	inline void DistributionComponent::UpdateConsumptionValue(std::size_t inputIndex, Nz::UInt32 consumption)
	{
		NazaraAssert(inputIndex < m_inputs.size());
		m_inputs[inputIndex].consumptionValue = consumption;
	}

	inline void DistributionComponent::UpdateProductionValue(std::size_t outputIndex, Nz::UInt32 production)
	{
		NazaraAssert(outputIndex < m_outputs.size());
		m_outputs[outputIndex].productionValue = production;
	}

	inline void DistributionComponent::ClearDistributedValues()
	{
		m_distributedValues.fill(0);
	}

	inline void DistributionComponent::IncrementDistributedValue(DistributionType type, Nz::UInt32 value)
	{
		m_distributedValues[type] += value;
	}
}
