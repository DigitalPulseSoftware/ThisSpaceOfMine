// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
#include <CommonLib/Direction.hpp>
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
	
	auto Planet::ComputeGridCell(const Nz::Vector3f& position) -> std::optional<GridCellIntersection>
	{
		Nz::Vector3f outsideNormal = Nz::Vector3f::Normalize(position - GetCenter());

		// Compute direction
		Direction closestDir;
		float closestDirDot = -1.f;
		for (auto&& [direction, normal] : s_dirNormals.iter_kv())
		{
			if (float dot = normal.DotProduct(outsideNormal); dot >= closestDirDot)
			{
				closestDir = direction;
				closestDirDot = dot;
			}
		}

		// Compute height
		auto& gridVec = m_grids[closestDir];
		std::size_t gridIndex = 0;
		for (; gridIndex < gridVec.size(); ++gridIndex)
		{
			float height = gridIndex * m_tileSize + 1.f;
			float dist = sdRoundBox(position, Nz::Vector3f(height), m_cornerRadius);
			if (dist - m_tileSize * 0.1f <= m_tileSize)
				break;
		}

		if (gridIndex >= gridVec.size())
		{
			fmt::print("grid out of bounds (dist: {})\n", sdRoundBox(position, Nz::Vector3f((gridVec.size() - 1) * m_tileSize + 1.f), m_cornerRadius));
			return std::nullopt;
		}

		VoxelGrid& grid = *gridVec[gridIndex];

		float gridHeight = gridIndex * m_tileSize + 1.f;

		float distToCenter = std::max({
			std::abs(position.x - GetCenter().x),
			std::abs(position.y - GetCenter().y),
			std::abs(position.z - GetCenter().z),
		});

		float innerReductionSize = std::max(m_tileSize + gridHeight - std::max(m_cornerRadius, 1.f), 0.f);
		Nz::Boxf innerBox(GetCenter() - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		//debugDrawer.DrawBox(innerBox, Nz::Color::Red());

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f rayNormal = Nz::Vector3f::Normalize(position - innerPos);

		Nz::Boxf box(GetCenter() - Nz::Vector3f(gridHeight), Nz::Vector3f(gridHeight * 2.f + m_tileSize));
		//debugDrawer.DrawBox(box, Nz::Color::Gray());
		Nz::Rayf ray(innerPos + rayNormal * gridHeight * 2.f, -rayNormal);

		//debugDrawer.DrawBox(box, Nz::Color::Green());

		//debugDrawer.DrawLine(position, position + outsideNormal, Nz::Color::Blue());

		float closest, furthest;
		if (!ray.Intersect(box, &closest, &furthest))
		{
			fmt::print("ray intersection failed\n");
			return std::nullopt;
		}

		//debugDrawer.DrawLine(ray.origin, ray.GetPoint(closest), Nz::Color::Magenta());

		//debugDrawer.DrawLine(ray.origin, ray.GetPoint(closest), Nz::Color::Red());

		Nz::Quaternionf rotation = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), s_dirNormals[closestDir]);

		Nz::Matrix4f transform = Nz::Matrix4f::Transform(rotation * Nz::Vector3f::Up() * gridHeight, rotation);
		Nz::Matrix4f transformInverse = Nz::Matrix4f::TransformInverse(rotation * Nz::Vector3f::Up() * gridHeight, rotation);

		Nz::Vector3f hitPoint = transformInverse * (ray.GetPoint(closest) - GetCenter());
		float x = std::floor(grid.GetWidth() * 0.5f + hitPoint.x / m_tileSize);
		float y = std::floor(grid.GetHeight() * 0.5f + hitPoint.z / m_tileSize);

		//fmt::print("x: {}, y: {}\n", x, y);

		float xOffset = x * m_tileSize - grid.GetWidth() * m_tileSize * 0.5f;
		float yHeight = m_tileSize;
		float zOffset = y * m_tileSize - grid.GetHeight() * m_tileSize * 0.5f;

		int xGrid = std::clamp(static_cast<int>(x), 0, static_cast<int>(grid.GetWidth()));
		int yGrid = std::clamp(static_cast<int>(y), 0, static_cast<int>(grid.GetHeight()));

		return GridCellIntersection{
			&grid,
			closestDir,
			Nz::SafeCast<std::size_t>(xGrid),
			Nz::SafeCast<std::size_t>(yGrid),
			gridHeight
		};
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
				VoxelBlock cell;
				if (i >= maxHeight)
					cell = VoxelBlock::Empty;
				else if (i == maxHeight - 1)
					cell = VoxelBlock::Grass;
				else if (i >= maxHeight - 3)
					cell = VoxelBlock::Dirt;
				else
					cell = VoxelBlock::Stone;

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
