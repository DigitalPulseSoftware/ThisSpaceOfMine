// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Ship.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/GameConstants.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Ship::Ship(const BlockLibrary& blockLibrary, const Nz::Vector3ui& gridSize, float tileSize) :
	ChunkContainer(tileSize),
	m_chunk(*this, { 0, 0, 0 }, gridSize, tileSize)
	{
		SetupChunk(blockLibrary);
	}

	float Ship::ComputeGravityAcceleration(const Nz::Vector3f& /*position*/) const
	{
		return Constants::ShipGravityAcceleration;
	}

	Nz::Vector3f Ship::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		return Nz::Vector3f::Up();
	}

	void Ship::SetupChunk(const BlockLibrary& blockLibrary)
	{
		constexpr unsigned int freespace = 5;

		BlockIndex hullIndex = blockLibrary.GetBlockIndex("hull");
		if (hullIndex == InvalidBlockIndex)
			return;

		constexpr unsigned int boxSize = 12;
		constexpr unsigned int heightSize = 5;
		Nz::Vector3ui startPos = m_chunk.GetSize() / 2 - Nz::Vector3ui(boxSize / 2, boxSize / 2, heightSize / 2);

		for (unsigned int z = 0; z < heightSize; ++z)
		{
			for (unsigned int y = 0; y < boxSize; ++y)
			{
				for (unsigned int x = 0; x < boxSize; ++x)
				{
					if (x != 0 && x != boxSize - 1 &&
					    y != 0 && y != boxSize - 1 &&
					    z != 0 && z != heightSize - 1)
					{
						continue;
					}

					m_chunk.UpdateBlock(startPos + Nz::Vector3ui{ x, y, z }, hullIndex);
				}
			}
		}
	}
}
