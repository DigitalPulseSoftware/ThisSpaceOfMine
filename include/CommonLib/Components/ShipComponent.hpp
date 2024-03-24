// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <NazaraUtils/MovablePtr.hpp>
#include <memory>

namespace tsom
{
	class Ship;

	struct ShipComponent
	{
		Nz::MovablePtr<Ship> ship;
	};
}

#include <CommonLib/Components/ShipComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP
