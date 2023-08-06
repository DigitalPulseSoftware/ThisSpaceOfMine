// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>
#include <fmt/std.h>

namespace tsom
{
	std::shared_ptr<Nz::JoltCollider3D> DeformedChunk::BuildCollider() const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(Nz::Matrix4f::Identity(), indices, vertices);
		if (indices.empty())
			return nullptr;

		return std::make_shared<Nz::JoltMeshCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size(), indices.data(), indices.size());
	}

	std::optional<Nz::Vector3ui> DeformedChunk::ComputeCoordinates(const Nz::Vector3f& position) const
	{
		Nz::Vector3f outsideNormal = Nz::Vector3f::Normalize(position - m_deformationCenter);

		// Compute direction
		Direction closestDir = DirectionFromNormal(outsideNormal);

		// Compute height
		std::size_t z = 0;
		for (; z < m_size.z; ++z)
		{
			float depth = z * m_cellSize;
			float dist = sdRoundBox(position, Nz::Vector3f(depth), m_deformationRadius);
			if (dist < 0.f)
				break;
		}

		if (z >= m_size.z)
		{
			fmt::print("grid out of bounds (dist: {})\n", sdRoundBox(position, Nz::Vector3f((m_size.z - 1) * m_cellSize), m_deformationRadius));
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

		Nz::Vector3f hitPoint = ray.GetPoint(closest);
		hitPoint += { 0.5f * m_size.x * m_cellSize, 0.5f * m_size.y * m_cellSize, 0.5f * m_size.z * m_cellSize };

		Nz::Vector3f indices = hitPoint / m_cellSize;
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_size.x || pos.y >= m_size.y || pos.z >= m_size.z)
			return std::nullopt;

		return pos;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> DeformedChunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		float fX = indices.x * m_cellSize - m_size.x * 0.5f * m_cellSize;
		float fY = indices.y * m_cellSize - m_size.y * 0.5f * m_cellSize;
		float fZ = indices.z * m_cellSize - m_size.z * 0.5f * m_cellSize;

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

		for (auto& position : corners)
			position = DeformPosition(position);

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
