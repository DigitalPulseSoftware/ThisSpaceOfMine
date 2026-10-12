// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline std::mt19937_64& RandomNumberGenerator::MersenneTwister64()
	{
		return m_mersenneTwister;
	}

	template<std::floating_point T>
	T RandomNumberGenerator::RandomFloat(T minValue, T maxValue)
	{
		std::uniform_real_distribution<T> dis(minValue, maxValue);
		return dis(m_mersenneTwister);
	}

	template<std::integral T>
	T RandomNumberGenerator::RandomInteger(T minValue, T maxValue)
	{
		std::uniform_int_distribution<T> dis(minValue, maxValue);
		return dis(m_mersenneTwister);
	}
}
