// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <memory>

namespace tsom
{
	struct ShipComponent
	{
		std::unique_ptr<Ship> ship;
		std::array<std::unique_ptr<ChunkEntities>, Constants::MaxChunkLayerCount> shipEntities;
	};
}

#include <CommonLib/Components/ShipComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_SHIPCOMPONENT_HPP
