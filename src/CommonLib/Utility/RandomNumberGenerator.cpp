// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utility/RandomNumberGenerator.hpp>

namespace tsom
{
	RandomNumberGenerator::RandomNumberGenerator()
	{
		std::random_device rd;
		m_mersenneTwister.seed(rd());
	}

	RandomNumberGenerator& RandomNumberGenerator::GetThreadGenerator()
	{
		thread_local RandomNumberGenerator generator;
		return generator;
	}
}
