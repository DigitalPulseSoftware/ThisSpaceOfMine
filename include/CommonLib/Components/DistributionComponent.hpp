// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityReference.hpp>
#include <CommonLib/GasType.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/Signal.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <span>
#include <variant>

namespace tsom
{
	enum class DistributionType
	{
		Electrical,
		Gas,

		Max = Gas
	};

	struct ElectricalQuantity
	{
		inline void Clear();
		inline void ComputeMin(const ElectricalQuantity& val1, const ElectricalQuantity& val2);

		inline ElectricalQuantity& operator+=(const ElectricalQuantity& quantity);

		Nz::UInt32 energy = 0; //< kWh
	};

	struct GasQuantity
	{
		inline void Clear();
		inline void ComputeMin(const GasQuantity& val1, const GasQuantity& val2);

		inline Nz::UInt32 Get(GasType type) const;
		inline Nz::UInt32 Increment(GasType type, Nz::UInt32 quantity);
		inline void Set(GasType type, Nz::UInt32 quantity);

		inline GasQuantity& operator+=(const GasQuantity& quantity);

		struct Entry
		{
			GasType type;
			Nz::UInt32 quantity = 0; //< liter
		};

		Nz::HybridVector<Entry, 3> gases;
	};

	using DistributionQuantity = std::variant<ElectricalQuantity, GasQuantity>;

	constexpr Nz::EnumArray<DistributionType, DistributionQuantity(*)()> s_defaultDistributionQuantityBuilder = {
		[]() -> DistributionQuantity { return ElectricalQuantity{ 0 }; }, // DistributionType::Electrical
		[]() -> DistributionQuantity { return GasQuantity{}; },           // DistributionType::Gas
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

			inline const DistributionQuantity& GetConsumptionValue(std::size_t inputIndex) const;

			inline const DistributionQuantity& GetDistributedValue(DistributionType type) const;

			inline entt::handle GetInputConnectedEntity(std::size_t inputIndex) const;
			inline std::size_t GetInputConnectedPort(std::size_t inputIndex) const;
			inline std::size_t GetInputCount() const;
			inline DistributionType GetInputType(std::size_t inputIndex) const;

			inline entt::handle GetOutputConnectedEntity(std::size_t outputIndex) const;
			inline std::size_t GetOutputConnectedPort(std::size_t outputIndex) const;
			inline std::size_t GetOutputCount() const;
			inline DistributionType GetOutputType(std::size_t outputIndex) const;

			inline const DistributionQuantity& GetProductionValue(std::size_t outputIndex) const;

			inline bool IsInputConnected(std::size_t inputIndex) const;
			inline bool IsOutputConnected(std::size_t outputIndex) const;

			inline void UpdateConsumptionValue(std::size_t inputIndex, DistributionQuantity consumption);
			inline void UpdateProductionValue(std::size_t outputIndex, DistributionQuantity production);

			DistributionComponent& operator=(const DistributionComponent&) = delete;
			DistributionComponent& operator=(DistributionComponent&&) noexcept = default;

			NazaraSignal(OnInputChanged, DistributionComponent*, std::size_t /*inputIndex*/);
			NazaraSignal(OnOutputChanged, DistributionComponent*, std::size_t /*outputIndex*/);

			static void Connect(entt::handle sourceEntity, entt::handle targetEntity, std::size_t sourceOutputPort, std::size_t targetInputPort);
			static void Connect(entt::handle sourceEntity, DistributionComponent& sourceEntityDistribution, entt::handle targetEntity, DistributionComponent& targetEntityDistribution, std::size_t sourceOutputPort, std::size_t targetInputPort);

		private:
			void ClearDistributedValues();

			inline void ConnectInput(std::size_t inputIndex, entt::handle entity, std::size_t outputIndex);
			inline void ConnectOutput(std::size_t outputIndex, entt::handle entity, std::size_t inputIndex);

			inline void DisconnectInput(std::size_t inputIndex);
			inline void DisconnectOutput(std::size_t outputIndex);

			void IncrementDistributedValue(DistributionType type, const DistributionQuantity& value);
			inline void TriggerDistributionCallback(entt::handle entity);

			struct Port
			{
				DistributionType type;
				EntityReference connectedEntity;
				std::size_t connectedPort;
			};

			struct InputPort : Port
			{
				DistributionQuantity consumptionValue;
			};

			struct OutputPort : Port
			{
				DistributionQuantity productionValue;
			};

			Nz::EnumArray<DistributionType, DistributionQuantity> m_distributedValues;
			Nz::HybridVector<InputPort, 3> m_inputs;
			Nz::HybridVector<OutputPort, 3> m_outputs;
			DistributionCallback m_distributionCallback;
	};
}

#include <CommonLib/Components/DistributionComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_DISTRIBUTIONCOMPONENT_HPP
