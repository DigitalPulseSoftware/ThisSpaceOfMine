// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_RANDOMNUMBERGENERATOR_HPP
#define TSOM_COMMONLIB_UTILITY_RANDOMNUMBERGENERATOR_HPP

#include <CommonLib/Export.hpp>
#include <concepts>
#include <random>

namespace tsom
{
	class TSOM_COMMONLIB_API RandomNumberGenerator
	{
		public:
			RandomNumberGenerator();
			RandomNumberGenerator(const RandomNumberGenerator&) = delete;
			RandomNumberGenerator(RandomNumberGenerator&&) = delete;
			~RandomNumberGenerator() = default;

			inline std::mt19937_64& MersenneTwister64();

			template<std::floating_point T> T RandomFloat(T minValue, T maxValue);
			template<std::integral T> T RandomInteger(T minValue, T maxValue);

			RandomNumberGenerator& operator=(const RandomNumberGenerator&) = delete;
			RandomNumberGenerator& operator=(RandomNumberGenerator&&) = delete;

			static RandomNumberGenerator& GetThreadGenerator();

		private:
			std::mt19937_64 m_mersenneTwister;
	};
}

#include <CommonLib/Utility/RandomNumberGenerator.inl>

#endif // TSOM_COMMONLIB_UTILITY_RANDOMNUMBERGENERATOR_HPP
