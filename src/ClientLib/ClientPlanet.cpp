// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientPlanet.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <Nazara/Math/Ray.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	ClientPlanet::ClientPlanet(std::size_t gridDims, float tileSize, float cornerRadius) :
	Planet(gridDims, tileSize, cornerRadius)
	{
	}

	std::shared_ptr<Nz::GraphicalMesh> ClientPlanet::BuildGfxMesh()
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(indices, vertices);

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, indices.size(), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Color_UV), vertices.size(), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();
		staticMesh->SetPrimitiveMode(Nz::PrimitiveMode::LineList);

		return Nz::GraphicalMesh::BuildFromMesh(*Nz::Mesh::Build(staticMesh));
	}


	std::tuple<std::size_t, std::size_t, VoxelGrid*> ClientPlanet::ComputeGridCell(const Nz::Vector3f& position, const Nz::Vector3f& normal, Nz::DebugDrawer& debugDrawer)
	{
		Nz::Vector3f insidePos = position - normal * m_tileSize * 0.25f;
		Nz::Vector3f outsideNormal = Nz::Vector3f::Normalize(insidePos - GetCenter());

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
			float dist = sdRoundBox(insidePos, Nz::Vector3f(height), m_cornerRadius);
			if (dist - m_tileSize * 0.1f <= m_tileSize)
				break;
		}

		if (gridIndex >= gridVec.size())
		{
			fmt::print("grid out of bounds (dist: {})\n", sdRoundBox(insidePos, Nz::Vector3f((gridVec.size() - 1) * m_tileSize + 1.f), m_cornerRadius));
			return { 0, 0, nullptr };
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

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(insidePos, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f rayNormal = Nz::Vector3f::Normalize(insidePos - innerPos);

		Nz::Boxf box(GetCenter() - Nz::Vector3f(gridHeight), Nz::Vector3f(gridHeight * 2.f + m_tileSize));
		//debugDrawer.DrawBox(box, Nz::Color::Gray());
		Nz::Rayf ray(innerPos + rayNormal * gridHeight * 2.f, -rayNormal);

		//debugDrawer.DrawBox(box, Nz::Color::Green());

		//debugDrawer.DrawLine(position, position + outsideNormal, Nz::Color::Blue());

		float closest, furthest;
		if (!ray.Intersect(box, &closest, &furthest))
		{
			fmt::print("ray intersection failed\n");
			return { 0, 0, nullptr };
		}

		//debugDrawer.DrawLine(ray.origin, ray.GetPoint(closest), Nz::Color::Magenta());

		//debugDrawer.DrawLine(ray.origin, ray.GetPoint(closest), Nz::Color::Red());

		Nz::Quaternionf rotation = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), s_dirNormals[closestDir]);

		Nz::Matrix4f transform = Nz::Matrix4f::Transform(rotation * Nz::Vector3f::Up() * gridHeight, rotation);
		Nz::Matrix4f transformInverse = Nz::Matrix4f::TransformInverse(rotation * Nz::Vector3f::Up() * gridHeight, rotation);

		Nz::Vector3f hitPoint = transformInverse * (ray.GetPoint(closest) - GetCenter());
		float x = std::floor(grid.GetWidth() * 0.5f + hitPoint.x / m_tileSize);
		float y = std::floor(grid.GetHeight() * 0.5f + hitPoint.z / m_tileSize);

		fmt::print("x: {}, y: {}\n", x, y);

		float xOffset = x * m_tileSize - grid.GetWidth() * m_tileSize * 0.5f;
		float yHeight = m_tileSize;
		float zOffset = y * m_tileSize - grid.GetHeight() * m_tileSize * 0.5f;

		debugDrawer.DrawLine(DeformPosition(transform * Nz::Vector3f(xOffset, yHeight, zOffset)),                           DeformPosition(transform * Nz::Vector3f(xOffset, yHeight, zOffset + m_tileSize)), Nz::Color::Green());
		debugDrawer.DrawLine(DeformPosition(transform * Nz::Vector3f(xOffset, yHeight, zOffset + m_tileSize)),              DeformPosition(transform * Nz::Vector3f(xOffset + m_tileSize, yHeight, zOffset + m_tileSize)), Nz::Color::Green());
		debugDrawer.DrawLine(DeformPosition(transform * Nz::Vector3f(xOffset + m_tileSize, yHeight, zOffset + m_tileSize)), DeformPosition(transform * Nz::Vector3f(xOffset + m_tileSize, yHeight, zOffset)), Nz::Color::Green());
		debugDrawer.DrawLine(DeformPosition(transform * Nz::Vector3f(xOffset + m_tileSize, yHeight, zOffset)),              DeformPosition(transform * Nz::Vector3f(xOffset, yHeight, zOffset)), Nz::Color::Green());

		//debugDrawer.DrawBox(Nz::Boxf(x * m_tileSize, gridSize, y * m_tileSize, m_tileSize, m_tileSize, m_tileSize), Nz::Color::Green());

		int xGrid = std::clamp(static_cast<int>(x), 0, static_cast<int>(grid.GetWidth()));
		int yGrid = std::clamp(static_cast<int>(y), 0, static_cast<int>(grid.GetHeight()));

		return { Nz::SafeCast<std::size_t>(xGrid), Nz::SafeCast<std::size_t>(yGrid), &grid };
	}
}
