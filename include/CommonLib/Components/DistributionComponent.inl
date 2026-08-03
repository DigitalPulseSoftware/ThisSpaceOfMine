// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/GameConstants.hpp>
#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	inline void ElectricalQuantity::Clear()
	{
		energy = 0;
	}

	inline void ElectricalQuantity::ComputeMin(const ElectricalQuantity& val1, const ElectricalQuantity& val2)
	{
		energy = std::min(val1.energy, val2.energy);
	}

	inline ElectricalQuantity& ElectricalQuantity::operator+=(const ElectricalQuantity& quantity)
	{
		energy += quantity.energy;
		return *this;
	}

	inline void GasQuantity::Clear()
	{
		gases.clear();
	}

	inline void GasQuantity::ComputeMin(const GasQuantity& val1, const GasQuantity& val2)
	{
		Clear();
		for (auto&& [gasType, quantity] : val1.gases)
			Increment(gasType, std::min(quantity, val2.Get(gasType)));
	}

	inline Nz::UInt32 GasQuantity::Get(GasType type) const
	{
		auto it = std::find_if(gases.begin(), gases.end(), [&](const GasQuantity::Entry& entry) { return entry.type == type; });
		if (it != gases.end())
			return it->quantity;
		else
			return 0;
	}

	inline Nz::UInt32 GasQuantity::Increment(GasType type, Nz::UInt32 quantity)
	{
		auto it = std::find_if(gases.begin(), gases.end(), [&](const GasQuantity::Entry& entry) { return entry.type == type; });
		if (it != gases.end())
		{
			it->quantity += quantity;
			return it->quantity;
		}
		else if (quantity > 0)
			gases.push_back({ type, quantity });

		return quantity;
	}

	inline void GasQuantity::Set(GasType type, Nz::UInt32 quantity)
	{
		auto it = std::find_if(gases.begin(), gases.end(), [&](const GasQuantity::Entry& entry) { return entry.type == type; });
		if (it != gases.end())
		{
			if (quantity > 0)
				it->quantity = quantity;
			else
				gases.erase(it);
		}
		else if (quantity > 0)
			gases.push_back({ type, quantity });
	}

	inline GasQuantity& GasQuantity::operator+=(const GasQuantity& quantity)
	{
		for (auto&& [type, gasQuantity] : quantity.gases)
			Increment(type, gasQuantity);

		return *this;
	}

	inline DistributionComponent::DistributionComponent(std::span<DistributionType> inputs, std::span<DistributionType> outputs)
	{
		m_inputs.reserve(inputs.size());
		for (DistributionType distributionType : inputs)
		{
			auto& port = m_inputs.emplace_back();
			port.type = distributionType;
			port.consumptionValue = s_defaultDistributionQuantityBuilder[distributionType]();
		}

		m_outputs.reserve(inputs.size());
		for (DistributionType distributionType : outputs)
		{
			auto& port = m_outputs.emplace_back();
			port.type = distributionType;
			port.productionValue = s_defaultDistributionQuantityBuilder[distributionType]();
		}

		for (auto&& [type, quantity] : m_distributedValues.iter_kv())
			quantity = s_defaultDistributionQuantityBuilder[type]();
	}

	inline void DistributionComponent::BindDistributionCallback(DistributionCallback&& callback)
	{
		m_distributionCallback = std::move(callback);
	}

	inline const DistributionQuantity& DistributionComponent::GetConsumptionValue(std::size_t inputIndex) const
	{
		NazaraAssert(inputIndex < m_inputs.size());
		return m_inputs[inputIndex].consumptionValue;
	}

	inline const DistributionQuantity& DistributionComponent::GetDistributedValue(DistributionType type) const
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

	inline const DistributionQuantity& DistributionComponent::GetProductionValue(std::size_t outputIndex) const
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

	inline void DistributionComponent::UpdateConsumptionValue(std::size_t inputIndex, DistributionQuantity consumption)
	{
		NazaraAssert(inputIndex < m_inputs.size());
		m_inputs[inputIndex].consumptionValue = std::move(consumption);
	}

	inline void DistributionComponent::UpdateProductionValue(std::size_t outputIndex, DistributionQuantity production)
	{
		NazaraAssert(outputIndex < m_outputs.size());
		m_outputs[outputIndex].productionValue = std::move(production);
	}

	inline void DistributionComponent::ConnectInput(std::size_t inputIndex, entt::handle entity, std::size_t outputIndex)
	{
		NazaraAssert(inputIndex < m_inputs.size());
		m_inputs[inputIndex].connectedEntity = entity;
		m_inputs[inputIndex].connectedPort = outputIndex;

		OnInputChanged(this, inputIndex);
	}

	inline void DistributionComponent::ConnectOutput(std::size_t outputIndex, entt::handle entity, std::size_t inputIndex)
	{
		NazaraAssert(outputIndex < m_outputs.size());
		m_outputs[outputIndex].connectedEntity = entity;
		m_outputs[outputIndex].connectedPort = inputIndex;

		OnOutputChanged(this, outputIndex);
	}

	inline void DistributionComponent::DisconnectInput(std::size_t inputIndex)
	{
		NazaraAssert(inputIndex < m_inputs.size());
		m_inputs[inputIndex].connectedEntity = EntityReference{};

		OnInputChanged(this, inputIndex);
	}

	inline void DistributionComponent::DisconnectOutput(std::size_t outputIndex)
	{
		NazaraAssert(outputIndex < m_outputs.size());
		m_outputs[outputIndex].connectedEntity = EntityReference{};

		OnOutputChanged(this, outputIndex);
	}

	void DistributionComponent::TriggerDistributionCallback(entt::handle entity)
	{
		if (m_distributionCallback)
			m_distributionCallback(entity);
	}
}
