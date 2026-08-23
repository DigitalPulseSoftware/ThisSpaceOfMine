// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DISTRIBUTION_HPP
#define TSOM_COMMONLIB_DISTRIBUTION_HPP

#include <CommonLib/GasType.hpp>
#include <Nazara/Core/Color.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <string_view>
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

		inline void Minimize(const ElectricalQuantity& quantity);

		inline ElectricalQuantity& operator+=(const ElectricalQuantity& quantity);

		Nz::UInt32 energy = 0; //< kWh
	};

	struct GasQuantity
	{
		inline void Clear();
		inline void ComputeMin(const GasQuantity& val1, const GasQuantity& val2);

		inline Nz::UInt32 Get(GasType type) const;
		inline Nz::UInt32 Increment(GasType type, Nz::UInt32 quantity);
		inline void Minimize(const GasQuantity& quantity);
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

	using DistributionQuantityBuilder = DistributionQuantity(*)();

	struct DistributionData
	{
		std::string_view name;
		Nz::Color color;
		DistributionQuantityBuilder zeroBuilder;
	};

	constexpr Nz::EnumArray<DistributionType, DistributionData> s_distributionData = {
		"Electrical", Nz::Color::Blue(),  []() -> DistributionQuantity { return ElectricalQuantity{0}; }, // DistributionType::Electrical
		"Gas",        Nz::Color::Green(), []() -> DistributionQuantity { return GasQuantity{}; },         // DistributionType::Gas
	};
}

#include <CommonLib/Distribution.inl>

#endif // TSOM_COMMONLIB_DISTRIBUTION_HPP
