// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTPLANET_HPP
#define TSOM_CLIENTLIB_CLIENTPLANET_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Planet.hpp>
#include <tsl/hopscotch_map.h>

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientPlanet : public Planet
	{
		public:
			using Planet::Planet;
			ClientPlanet(const ClientPlanet&) = delete;
			ClientPlanet(ClientPlanet&&) = delete;
			~ClientPlanet() = default;

			Chunk& AddChunk(Nz::UInt16 networkIndex, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);

			inline Chunk* GetChunkByNetworkIndex(Nz::UInt16 networkIndex);
			inline const Chunk* GetChunkByNetworkIndex(Nz::UInt16 networkIndex) const;
			inline Nz::UInt16 GetChunkNetworkIndex(const Chunk* chunk) const;

			void RemoveChunk(Nz::UInt16 networkIndex);

			ClientPlanet& operator=(const ClientPlanet&) = delete;
			ClientPlanet& operator=(ClientPlanet&&) = delete;

		private:
			tsl::hopscotch_map<Nz::UInt16 /*networkIndex*/, Chunk* /*chunk*/> m_chunkByNetworkIndex;
			tsl::hopscotch_map<const Chunk* /*chunk*/, Nz::UInt16 /*networkIndex*/> m_chunkNetworkIndices;
	};
}

#include <ClientLib/ClientPlanet.inl>

#endif // TSOM_CLIENTLIB_CLIENTPLANET_HPP
