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

	inline void ElectricalQuantity::Minimize(const ElectricalQuantity& quantity)
	{
		energy = std::min(energy, quantity.energy);
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

	inline void GasQuantity::Minimize(const GasQuantity& quantity)
	{
		for (auto it = gases.begin(); it != gases.end();)
		{
			Nz::UInt32 minQuantity = std::min(it->quantity, quantity.Get(it->type));
			if (minQuantity > 0)
			{
				it->quantity = minQuantity;
				++it;
			}
			else
				it = gases.erase(it);
		}
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
}
