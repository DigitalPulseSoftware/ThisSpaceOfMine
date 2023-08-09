// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientPlanet.hpp>

namespace tsom
{
	ClientPlanet::ClientPlanet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius) :
	Planet(gridSize, tileSize, cornerRadius)
	{
	}

	Chunk& ClientPlanet::AddChunk(Nz::UInt16 networkIndex, const Nz::Vector3ui& indices, const Nz::FunctionRef<void(VoxelBlock* blocks)>& initCallback)
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
