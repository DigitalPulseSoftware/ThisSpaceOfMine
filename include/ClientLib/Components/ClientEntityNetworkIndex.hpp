// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_CLIENTENTITYNETWORKINDEX_HPP
#define TSOM_CLIENTLIB_COMPONENTS_CLIENTENTITYNETWORKINDEX_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	struct ClientEntityNetworkIndex
	{
		Packets::Helper::EntityId networkIndex;
	};
}

#endif // TSOM_CLIENTLIB_COMPONENTS_CLIENTENTITYNETWORKINDEX_HPP
