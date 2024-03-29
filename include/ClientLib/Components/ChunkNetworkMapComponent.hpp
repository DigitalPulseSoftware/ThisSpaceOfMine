// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_CHUNKNETWORKMAPCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_CHUNKNETWORKMAPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <tsl/hopscotch_map.h>

namespace tsom
{
	class Chunk;

	struct ChunkNetworkMapComponent
	{
		tsl::hopscotch_map<Nz::UInt16 /*networkIndex*/, Chunk* /*chunk*/> chunkByNetworkIndex;
		tsl::hopscotch_map<const Chunk* /*chunk*/, Nz::UInt16 /*networkIndex*/> chunkNetworkIndices;
	};
}

#include <ClientLib/Components/ChunkNetworkMapComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_CHUNKNETWORKMAPCOMPONENT_HPP
