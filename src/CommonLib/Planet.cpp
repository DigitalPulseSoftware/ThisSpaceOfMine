// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
#include <CommonLib/Direction.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
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
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(indices, vertices);

#if 0
		return std::make_shared<Nz::JoltConvexHullCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size());
#else
		return std::make_shared<Nz::JoltMeshCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size(), indices.data(), indices.size());
#endif
	}

	void Planet::BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		float maxHeight = m_gridDimensions * m_tileSize * 0.5f;

		for (auto&& [direction, normal] : s_dirNormals.iter_kv())
		{
			if (direction == Direction::Left)
				continue;

			Nz::Quaternionf rotation = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), normal);

			for (std::size_t i = 0; i < m_grids[direction].size(); ++i)
			{
				float offset = i * m_tileSize + 1.f;
				Nz::Matrix4f transform = Nz::Matrix4f::Transform(rotation * Nz::Vector3f::Up() * offset, rotation);
				m_grids[direction][i]->BuildMesh(transform, m_tileSize, s_dirColors[direction], indices, vertices);
			}
		}

		Nz::Vector3f center = GetCenter();
		for (Nz::VertexStruct_XYZ_Color_UV& vert : vertices)
		{
			float distToCenter = std::max({
				std::abs(vert.position.x - center.x),
				std::abs(vert.position.y - center.y),
				std::abs(vert.position.z - center.z),
			});

			vert.color *= Nz::Color(std::clamp(distToCenter / maxHeight * 2.f, 0.f, 1.f));

			vert.position = DeformPosition(vert.position);
		}
	}

	Nz::Vector3f Planet::DeformPosition(const Nz::Vector3f& position)
	{
		Nz::Vector3f center = GetCenter();

		float distToCenter = std::max({
			std::abs(position.x - center.x),
			std::abs(position.y - center.y),
			std::abs(position.z - center.z),
		});

		float innerReductionSize = std::max(distToCenter - m_cornerRadius, 0.f);
		Nz::Boxf innerBox(center - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f normal = Nz::Vector3f::Normalize(position - innerPos);

		return innerPos + normal * std::min(m_cornerRadius, distToCenter);
	}

	void Planet::RebuildGrid()
	{
		for (auto& gridVec : m_grids)
		{
			gridVec.clear();

			std::size_t maxHeight = m_gridDimensions / 2;

			std::size_t gridSize = 1;
			for (std::size_t i = 0; i < m_gridDimensions; ++i)
			{
				VoxelCell cell;
				if (i >= maxHeight)
					cell = VoxelCell::Empty;
				else if (i == maxHeight - 1)
					cell = VoxelCell::Grass;
				else if (i >= maxHeight - 3)
					cell = VoxelCell::Dirt;
				else
					cell = VoxelCell::Stone;

				gridVec.emplace_back(std::make_unique<VoxelGrid>(gridSize, gridSize, cell));
				gridSize += 2;
			}
		}

		// Attach neighbors
		for (auto& gridVec : m_grids)
		{
			for (std::size_t i = 0; i < gridVec.size(); ++i)
			{
				if (i > 0)
					gridVec[i]->AttachNeighborGrid(Direction::Down, { -1, -1 }, gridVec[i - 1].get());

				if (i < gridVec.size() - 1)
					gridVec[i]->AttachNeighborGrid(Direction::Up, { 1, 1 }, gridVec[i + 1].get());
			}
		}
	}
}
