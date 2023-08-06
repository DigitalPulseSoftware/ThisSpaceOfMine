// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline Chunk* ClientPlanet::GetChunkByNetworkIndex(Nz::UInt16 networkIndex) const
	{
		auto it = m_chunkByNetworkIndex.find(networkIndex);
		if (it == m_chunkByNetworkIndex.end())
			return nullptr;

		return it->second;
	}

	inline Nz::UInt16 ClientPlanet::GetChunkNetworkIndex(const Chunk* chunk) const
	{
		return Nz::Retrieve(m_chunkNetworkIndices, chunk);
	}
}

