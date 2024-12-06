// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_SHIPEXTERIORCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_SHIPEXTERIORCOMPONENT_HPP

#include <ServerLib/ServerShipEnvironment.hpp>

namespace tsom
{
	class ServerShipEnvironment;

	struct ShipExteriorComponent
	{
		ServerShipEnvironment* ownerShip;

		NazaraSlot(ServerShipEnvironment, OnInteriorColliderUpdated, onInteriorColliderUpdated);
	};
}

#endif // TSOM_SERVERLIB_COMPONENTS_SHIPEXTERIORCOMPONENT_HPP
