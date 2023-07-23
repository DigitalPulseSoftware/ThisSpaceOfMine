// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>

namespace tsom
{
	std::shared_ptr<Nz::JoltCollider3D> DeformedChunk::BuildCollider(const Nz::Matrix4f& transformMatrix) const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(transformMatrix, Nz::Color::White(), indices, vertices);

		return std::make_shared<Nz::JoltMeshCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size(), indices.data(), indices.size());
	}

	void DeformedChunk::BuildMesh(const Nz::Matrix4f& transformMatrix, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices) const
	{
		Chunk::BuildMesh(transformMatrix, color, indices, vertices);

		for (auto& vertex : vertices)
			vertex.position = DeformPosition(vertex.position);
	}

	std::optional<Nz::Vector3ui> DeformedChunk::ComputeCoordinates(const Nz::Vector3f& position) const
	{
		Nz::Vector3f outsideNormal = Nz::Vector3f::Normalize(position - m_deformationCenter);

		// Compute direction
		Direction closestDir = DirectionFromNormal(outsideNormal);

		// Compute height
		std::size_t z = 0;
		for (; z < m_depth; ++z)
		{
			float depth = z * m_cellSize;
			float dist = sdRoundBox(position, Nz::Vector3f(depth), m_deformationRadius);
			if (dist < 0.f)
				break;
		}

		if (z >= m_depth)
		{
			fmt::print("grid out of bounds (dist: {})\n", sdRoundBox(position, Nz::Vector3f((m_depth - 1) * m_cellSize), m_deformationRadius));
			return std::nullopt;
		}

		float gridHeight = z * m_cellSize;

		float distToCenter = std::max({
			std::abs(position.x - m_deformationCenter.x),
			std::abs(position.y - m_deformationCenter.y),
			std::abs(position.z - m_deformationCenter.z),
		});

		float innerReductionSize = std::max(m_cellSize + gridHeight - std::max(m_deformationRadius, 1.f), 0.f);
		Nz::Boxf innerBox(m_deformationCenter - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		//debugDrawer.DrawBox(innerBox, Nz::Color::Red());

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f rayNormal = Nz::Vector3f::Normalize(position - innerPos);

		Nz::Boxf box(m_deformationCenter - Nz::Vector3f(gridHeight), Nz::Vector3f(gridHeight * 2.f + m_cellSize));
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

		Nz::Vector3f hitPoint = transformInverse * (ray.GetPoint(closest) - m_deformationCenter);
		Nz::Vector3f indices = hitPoint / m_cellSize;
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_width || pos.y >= m_height || pos.z >= m_depth)
			return std::nullopt;

		return pos;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> DeformedChunk::ComputeVoxelCorners(std::size_t x, std::size_t y, std::size_t z) const
	{
		float fX = x * m_cellSize;
		float fY = y * m_cellSize;
		float fZ = z * m_cellSize;

		Nz::Boxf box(fX, fZ, fY, m_cellSize, m_cellSize, m_cellSize);
		Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners {
			box.GetCorner(Nz::BoxCorner::FarLeftBottom),
			box.GetCorner(Nz::BoxCorner::FarLeftTop),
			box.GetCorner(Nz::BoxCorner::FarRightBottom),
			box.GetCorner(Nz::BoxCorner::FarRightTop),
			box.GetCorner(Nz::BoxCorner::NearLeftBottom),
			box.GetCorner(Nz::BoxCorner::NearLeftTop),
			box.GetCorner(Nz::BoxCorner::NearRightBottom),
			box.GetCorner(Nz::BoxCorner::NearRightTop)
		};

		if (x == 0)
		{
			corners[Nz::BoxCorner::NearLeftTop].x -= m_cellSize;
			corners[Nz::BoxCorner::FarLeftTop].x -= m_cellSize;
		}

		if (x == m_width - 1)
		{
			corners[Nz::BoxCorner::NearRightTop].x += m_cellSize;
			corners[Nz::BoxCorner::FarRightTop].x += m_cellSize;
		}

		if (y == 0)
		{
			corners[Nz::BoxCorner::FarLeftTop].z -= m_cellSize;
			corners[Nz::BoxCorner::FarRightTop].z -= m_cellSize;
		}

		if (y == m_height - 1)
		{
			corners[Nz::BoxCorner::NearLeftTop].z += m_cellSize;
			corners[Nz::BoxCorner::NearRightTop].z += m_cellSize;
		}

		for (Nz::Vector3f& corner : corners)
			corner = DeformPosition(corner);

		return corners;
	}

	Nz::Vector3f DeformedChunk::DeformPosition(const Nz::Vector3f& position) const
	{
		float distToCenter = std::max({
			std::abs(position.x - m_deformationCenter.x),
			std::abs(position.y - m_deformationCenter.y),
			std::abs(position.z - m_deformationCenter.z),
		});

		float innerReductionSize = std::max(distToCenter - m_deformationRadius, 0.f);
		Nz::Boxf innerBox(m_deformationCenter - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f normal = Nz::Vector3f::Normalize(position - innerPos);

		return innerPos + normal * std::min(m_deformationRadius, distToCenter);
	}
}
