// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Planet::Planet(std::size_t gridDims, float tileSize, float cornerRadius) :
	m_gridDimensions(gridDims),
	m_cornerRadius(cornerRadius),
	m_tileSize(tileSize)
	{
		RebuildGrid();
	}
	
	std::shared_ptr<Nz::JoltCollider3D> Planet::BuildCollider()
	{
		return m_chunk->BuildCollider(Nz::Matrix4f::Translate(Nz::Vector3f(m_gridDimensions * 0.5f * m_tileSize)));
	}

	std::optional<Nz::Vector3ui> Planet::ComputeGridCell(const Nz::Vector3f& position) const
	{
		return m_chunk->ComputeCoordinates(position);
	}

	void Planet::BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		m_chunk->BuildMesh(Nz::Matrix4f::Translate(Nz::Vector3f(m_gridDimensions * 0.5f * m_tileSize)), Nz::Color::White(), indices, vertices);

		float maxHeight = m_gridDimensions / 2 * m_tileSize;

		Nz::Vector3f center = GetCenter();
		for (Nz::VertexStruct_XYZ_Color_UV& vert : vertices)
		{
			float distToCenter = std::max({
				std::abs(vert.position.x - center.x),
				std::abs(vert.position.y - center.y),
				std::abs(vert.position.z - center.z),
			});

			vert.color *= Nz::Color(std::clamp(distToCenter / maxHeight * 2.f, 0.f, 1.f));
		}
	}

	void Planet::RebuildGrid()
	{
		constexpr std::size_t freeSpace = 10;

		m_chunk = std::make_unique<DeformedChunk>(m_gridDimensions, m_gridDimensions, m_gridDimensions, m_tileSize, Nz::Vector3f::Zero(), m_cornerRadius);
		for (std::size_t z = 0; z < m_gridDimensions; ++z)
		{
			for (std::size_t y = 0; y < m_gridDimensions; ++y)
			{
				for (std::size_t x = 0; x < m_gridDimensions; ++x)
				{
					if (x < freeSpace || x >= m_gridDimensions - freeSpace ||
						y < freeSpace || y >= m_gridDimensions - freeSpace ||
						z < freeSpace || z >= m_gridDimensions - freeSpace)
						continue;

					m_chunk->UpdateCell(x, y, z, VoxelBlock::Grass);
				}
			}
		}

		/*for (auto& gridVec : m_grids)
		{
			gridVec.clear();

			std::size_t maxHeight = m_gridDimensions / 2;

			std::size_t gridSize = 1;
			for (std::size_t i = 0; i < m_gridDimensions; ++i)
			{
				VoxelBlock cell;
				if (i >= maxHeight)
					cell = VoxelBlock::Empty;
				else if (i == maxHeight - 1)
					cell = VoxelBlock::Grass;
				else if (i >= maxHeight - 3)
					cell = VoxelBlock::Dirt;
				else
					cell = VoxelBlock::Stone;

				gridVec.emplace_back(std::make_unique<Chunk>(gridSize, gridSize, cell));
				gridSize += 2;
			}
		}*/
	}
}
