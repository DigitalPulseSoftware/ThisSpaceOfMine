// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_PLANETGRAVITYCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_PLANETGRAVITYCOMPONENT_HPP

#include <CommonLib/Export.hpp>

namespace tsom
{
	class Planet;

	struct PlanetGravityComponent
	{
		const Planet* planet = nullptr;
	};
}

#include <CommonLib/Components/PlanetGravityComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_ENTITYOWNERCOMPONENT_HPP
