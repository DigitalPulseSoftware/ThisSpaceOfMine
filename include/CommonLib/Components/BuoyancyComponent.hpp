// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_BUOYANCYCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_BUOYANCYCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <array>
#include <atomic>
#include <mutex>

namespace tsom
{
	struct BuoyancyComponent
	{
		struct FluidContact
		{
			Nz::UInt32 fluidBodyIndex;
			Nz::UInt32 fluidSubshapeID;
			Nz::UInt32 bodySubshapeID;
			Nz::Vector3f contactPos;
			unsigned int contactIndex = 0;
		};

		std::mutex mutex; // TODO: Use only atomics?
		std::array<FluidContact, 16> contacts;
		std::atomic_uint contactCount = 0u;
	};
}

#include <CommonLib/Components/BuoyancyComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_BUOYANCYCOMPONENT_HPP
