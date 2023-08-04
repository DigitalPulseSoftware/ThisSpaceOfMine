// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/FlatChunk.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>

namespace tsom
{
	std::shared_ptr<Nz::JoltCollider3D> FlatChunk::BuildCollider() const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(Nz::Matrix4f::Identity(), indices, vertices);

		return std::make_shared<Nz::JoltMeshCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size(), indices.data(), indices.size());
	}

	std::optional<Nz::Vector3ui> FlatChunk::ComputeCoordinates(const Nz::Vector3f& position) const
	{
		Nz::Vector3f indices = position / m_cellSize;
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_size.x || pos.y >= m_size.y || pos.z >= m_size.z)
			return std::nullopt;

		return pos;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> FlatChunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		float fX = indices.x * m_cellSize;
		float fY = indices.y * m_cellSize;
		float fZ = indices.z * m_cellSize;

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

		return corners;
	}
}


