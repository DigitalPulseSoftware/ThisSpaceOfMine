// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/DeformedChunk.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <numeric>

namespace tsom
{
	std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> DeformedChunk::BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale) const
	{
		auto corners = ComputeVoxelCorners(blockIndices);
		Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

		for (Nz::Vector3f& corner : corners)
			corner = (corner - blockCenter) * scale + blockCenter;

		return { std::make_shared<Nz::ConvexHullCollider3D>(corners.data(), corners.size()), blockCenter };
	}

	std::shared_ptr<Nz::Collider3D> DeformedChunk::BuildCollider(std::size_t layerIndex) const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::Vector3f> positions;
		std::vector<Nz::UInt32> triangleUserdata;

		auto AddFace = [&](BlockIndex blockContent, const Nz::Vector3ui& blockIndices, Direction direction)
		{
			Nz::UInt32 firstIndex = Nz::SafeCaster(positions.size());
			throw std::runtime_error("todo");
			/*for (const Nz::Vector3f& corner : faceCorners)
				positions.push_back(corner);*/

			indices.push_back(firstIndex);
			indices.push_back(firstIndex + 2);
			indices.push_back(firstIndex + 1);

			indices.push_back(firstIndex + 1);
			indices.push_back(firstIndex + 2);
			indices.push_back(firstIndex + 3);

			Nz::UInt32 localBlockIndex = GetBlockLocalIndex(blockIndices) * 6 + static_cast<Nz::UInt32>(direction);
			triangleUserdata.push_back(localBlockIndex);
			triangleUserdata.push_back(localBlockIndex);
		};

		BuildFaces(layerIndex, AddFace, true);
		if (indices.empty())
			return nullptr;

		Nz::MeshCollider3D::Settings meshSettings;
		meshSettings.indexCount = indices.size();
		meshSettings.indices = indices.data();
		meshSettings.vertexCount = positions.size();
		meshSettings.vertices = &positions[0];
		meshSettings.triangleUserdata = &triangleUserdata[0];

		return std::make_shared<Nz::MeshCollider3D>(meshSettings);
	}

	std::optional<Chunk::HitBlock> DeformedChunk::ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const
	{
		std::uint32_t remainder;
		const Nz::Collider3D* subCollider = collider.GetSubCollider(hitSubshapeId, remainder);
		if (!subCollider)
			return std::nullopt;

		Nz::UInt32 userdata = SafeCast<const Nz::MeshCollider3D*>(subCollider)->GetTriangleUserData(remainder);

		return HitBlock {
			.direction = static_cast<Direction>(userdata % 6),
			.blockIndices = GetBlockLocalIndices(userdata / 6)
		};
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> DeformedChunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = Chunk::ComputeVoxelCorners(indices);
		for (auto& position : corners)
			position = DeformPosition(position, m_deformationCenter, m_deformationRadius);

		return corners;
	}

	void DeformedChunk::DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		for (std::size_t i = 0; i < vertexCount; ++i)
		{
			Nz::Quaternionf rotation = GetNormalDeformation(positions[i], referenceNormal, m_deformationCenter, m_deformationRadius);
			normals[i] = rotation * normals[i];
		}
	}

	void DeformedChunk::DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		for (std::size_t i = 0; i < vertexCount; ++i)
		{
			Nz::Quaternionf rotation = GetNormalDeformation(positions[i], referenceNormal, m_deformationCenter, m_deformationRadius);
			normals[i] = rotation * normals[i];
			tangents[i] = rotation * tangents[i];
		}
	}

	bool DeformedChunk::DeformPositions(Nz::SparsePtr<Nz::Vector3f> positions, std::size_t positionCount) const
	{
		for (std::size_t i = 0; i < positionCount; ++i)
			positions[i] = DeformPosition(positions[i], m_deformationCenter, m_deformationRadius);

		return true;
	}
}
