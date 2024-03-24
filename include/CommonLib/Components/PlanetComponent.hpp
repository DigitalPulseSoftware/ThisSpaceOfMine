// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_PLANETCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_PLANETCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/MovablePtr.hpp>

namespace tsom
{
	class Planet;

	struct PlanetComponent
	{
		Nz::MovablePtr<Planet> planet;
	};
}

#include <CommonLib/Components/PlanetComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_PLANETCOMPONENT_HPP
