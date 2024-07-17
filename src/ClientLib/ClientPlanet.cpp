// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientPlanet.hpp>

namespace tsom
{
	Chunk& ClientPlanet::AddChunk(Nz::UInt16 networkIndex, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback)
	{
		Chunk& chunk = Planet::AddChunk(indices, initCallback);

		m_chunkByNetworkIndex.emplace(networkIndex, &chunk);
		m_chunkNetworkIndices.emplace(&chunk, networkIndex);

		return chunk;
	}

	void ClientPlanet::RemoveChunk(Nz::UInt16 networkIndex)
	{
		Chunk* chunk = Nz::Retrieve(m_chunkByNetworkIndex, networkIndex);

		m_chunkNetworkIndices.erase(chunk);
		m_chunkByNetworkIndex.erase(networkIndex);

		Planet::RemoveChunk(chunk->GetIndices());
	}
}
