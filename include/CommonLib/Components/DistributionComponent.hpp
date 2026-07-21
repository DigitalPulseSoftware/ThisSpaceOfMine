// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityReference.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/Signal.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <span>

namespace tsom
{
	enum class DistributionType
	{
		Electrical,

		Max = Electrical
	};

	class TSOM_COMMONLIB_API DistributionComponent
	{
		friend class DistributionSystem;

		public:
			using DistributionCallback = std::function<void(entt::handle)>;

			inline DistributionComponent(std::span<DistributionType> inputs, std::span<DistributionType> outputs);
			DistributionComponent(const DistributionComponent&) = delete;
			DistributionComponent(DistributionComponent&&) noexcept = default;
			~DistributionComponent() = default;

			inline void BindDistributionCallback(DistributionCallback&& callback);

			inline void ConnectInput(std::size_t inputIndex, entt::handle entity, std::size_t outputIndex);
			inline void ConnectOutput(std::size_t outputIndex, entt::handle entity, std::size_t inputIndex);

			inline Nz::UInt32 GetConsumptionValue(std::size_t inputIndex) const;

			inline Nz::UInt64 GetDistributedValue(DistributionType type) const;

			inline entt::handle GetInputConnectedEntity(std::size_t inputIndex) const;
			inline std::size_t GetInputConnectedPort(std::size_t inputIndex) const;
			inline std::size_t GetInputCount() const;
			inline DistributionType GetInputType(std::size_t inputIndex) const;

			inline entt::handle GetOutputConnectedEntity(std::size_t outputIndex) const;
			inline std::size_t GetOutputConnectedPort(std::size_t outputIndex) const;
			inline std::size_t GetOutputCount() const;
			inline DistributionType GetOutputType(std::size_t outputIndex) const;

			inline Nz::UInt32 GetProductionValue(std::size_t outputIndex) const;

			inline bool IsInputConnected(std::size_t inputIndex) const;
			inline bool IsOutputConnected(std::size_t outputIndex) const;

			inline void UpdateConsumptionValue(std::size_t inputIndex, Nz::UInt32 consumption);
			inline void UpdateProductionValue(std::size_t outputIndex, Nz::UInt32 production);

			DistributionComponent& operator=(const DistributionComponent&) = delete;
			DistributionComponent& operator=(DistributionComponent&&) noexcept = default;

			NazaraSignal(OnInputOutputChanged, DistributionComponent*);

		private:
			inline void ClearDistributedValues();
			inline void IncrementDistributedValue(DistributionType type, Nz::UInt32 value);
			inline void TriggerDistributionCallback(entt::handle entity);

			struct Port
			{
				DistributionType type;
				EntityReference connectedEntity;
				std::size_t connectedPort;
			};

			struct InputPort : Port
			{
				Nz::UInt32 consumptionValue = 0;
			};

			struct OutputPort : Port
			{
				Nz::UInt32 productionValue = 0;
			};

			Nz::EnumArray<DistributionType, Nz::UInt64> m_distributedValues;
			Nz::HybridVector<InputPort, 3> m_inputs;
			Nz::HybridVector<OutputPort, 3> m_outputs;
			DistributionCallback m_distributionCallback;
	};
}

#include <CommonLib/Components/DistributionComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP
