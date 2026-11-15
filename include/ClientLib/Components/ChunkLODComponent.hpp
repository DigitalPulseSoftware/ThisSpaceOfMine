// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_CHUNKLODCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_CHUNKLODCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/ChunkContainerLOD.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <memory>

namespace tsom
{
	struct ChunkLODComponent
	{
		struct LodEntry
		{
			std::shared_ptr<ChunkContainerLOD> chunkContainer; //< no container = disabled
			float screenSize;
		};

		Nz::FixedVector<LodEntry, Constants::MaxChunkLevelOfDetail> chunkContainerLOD;
		Nz::UInt8 activeLod = 0;
		float radius;
	};
}

#include <ClientLib/Components/ChunkLODComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_CHUNKLODCOMPONENT_HPP
