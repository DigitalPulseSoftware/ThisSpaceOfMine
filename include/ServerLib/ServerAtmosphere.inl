// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ServerAtmosphere::ServerAtmosphere()
	{
		m_gasAmount.fill(0);
	}

	inline bool ServerAtmosphere::DecreaseGasAmount(GasType type, Nz::UInt64 amount)
	{
		if (m_gasAmount[type] < amount)
		{
			m_gasAmount[type] = 0;
			return false;
		}

		m_gasAmount[type] -= amount;
		return true;
	}

	inline bool ServerAtmosphere::Exchange(const Nz::EnumArray<GasType, Nz::Int32>& amounts)
	{
		for (auto&& [gasType, amount] : amounts.iter_kv())
		{
			if (amount < 0 && m_gasAmount[gasType] < -amount)
				return false;
		}

		for (auto&& [gasType, amount] : amounts.iter_kv())
			m_gasAmount[gasType] += amount;

		return true;
	}

	inline Nz::UInt64 ServerAtmosphere::GetGasAmount(GasType type) const
	{
		return m_gasAmount[type];
	}

	inline const Nz::EnumArray<GasType, Nz::UInt64>& ServerAtmosphere::GetGasAmounts() const
	{
		return m_gasAmount;
	}

	inline void ServerAtmosphere::IncreaseGasAmount(GasType type, Nz::UInt64 amount)
	{
		m_gasAmount[type] += amount;
	}

	inline void ServerAtmosphere::SetGasAmount(GasType type, Nz::UInt64 millilitre)
	{
		m_gasAmount[type] = millilitre;
	}
}
