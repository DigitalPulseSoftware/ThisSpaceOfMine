// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Components/DistributionComponent.hpp>

namespace tsom
{
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
