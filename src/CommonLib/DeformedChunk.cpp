// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Core/VertexStruct.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <fmt/format.h>
#include <fmt/std.h>

namespace tsom
{
	std::shared_ptr<Nz::Collider3D> DeformedChunk::BuildCollider() const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::Vector3f> positions;

		auto AddVertices = [&](Nz::UInt32 count)
		{
			VertexAttributes vertexAttributes;

			vertexAttributes.firstIndex = Nz::SafeCast<Nz::UInt32>(positions.size());
			positions.resize(positions.size() + count);
			vertexAttributes.position = Nz::SparsePtr<Nz::Vector3f>(&positions[vertexAttributes.firstIndex]);

			return vertexAttributes;
		};

		BuildMesh(indices, m_deformationCenter, AddVertices);
		if (indices.empty())
			return nullptr;

		Nz::MeshCollider3D::Settings meshSettings;
		meshSettings.indexCount = indices.size();
		meshSettings.indices = indices.data();
		meshSettings.vertexCount = positions.size();
		meshSettings.vertices = &positions[0];

		return std::make_shared<Nz::MeshCollider3D>(meshSettings);
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
			float depth = z * m_blockSize;
			float dist = sdRoundBox(position, Nz::Vector3f(depth), m_deformationRadius);
			if (dist < 0.f)
				break;
		}

		if (z >= m_size.z)
		{
			fmt::print("grid out of bounds (dist: {})\n", sdRoundBox(position, Nz::Vector3f((m_size.z - 1) * m_blockSize), m_deformationRadius));
			return std::nullopt;
		}

		float gridHeight = z * m_blockSize;

		float distToCenter = std::max({
			std::abs(position.x - m_deformationCenter.x),
			std::abs(position.y - m_deformationCenter.y),
			std::abs(position.z - m_deformationCenter.z),
		});

		float innerReductionSize = std::max(m_blockSize + gridHeight - std::max(m_deformationRadius, 1.f), 0.f);
		Nz::Boxf innerBox(m_deformationCenter - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		//debugDrawer.DrawBox(innerBox, Nz::Color::Red());

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f rayNormal = Nz::Vector3f::Normalize(position - innerPos);

		Nz::Boxf box(m_deformationCenter - Nz::Vector3f(gridHeight), Nz::Vector3f(gridHeight * 2.f + m_blockSize));
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
		hitPoint += { 0.5f * m_size.x * m_blockSize, 0.5f * m_size.y * m_blockSize, 0.5f * m_size.z * m_blockSize };

		Nz::Vector3f indices = hitPoint / m_blockSize;
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_size.x || pos.y >= m_size.y || pos.z >= m_size.z)
			return std::nullopt;

		return pos;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> DeformedChunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		float fX = indices.x * m_blockSize;
		float fY = indices.y * m_blockSize;
		float fZ = indices.z * m_blockSize;

		Nz::Boxf box(fX, fZ, fY, m_blockSize, m_blockSize, m_blockSize);
		Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = box.GetCorners();

		for (auto& position : corners)
			position = DeformPosition(position, m_deformationCenter, m_deformationRadius);

		return corners;
	}
}
