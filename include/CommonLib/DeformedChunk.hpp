// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DEFORMEDCHUNK_HPP
#define TSOM_COMMONLIB_DEFORMEDCHUNK_HPP

#include <CommonLib/Chunk.hpp>

namespace tsom
{
	class DeformedChunk : public Chunk
	{
		public:
			inline DeformedChunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius);
			DeformedChunk(const DeformedChunk&) = delete;
			DeformedChunk(DeformedChunk&&) = delete;
			~DeformedChunk() = default;

			std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale = 1.f) const override;
			std::shared_ptr<Nz::Collider3D> BuildCollider() const override;

			std::optional<HitBlock> ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const override;
			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(const Nz::Vector3ui& indices) const override;

			void DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const override;
			void DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const override;
			bool DeformPositions(Nz::SparsePtr<Nz::Vector3f> positions, std::size_t positionCount) const override;

			inline void UpdateDeformationRadius(float deformationRadius);

			DeformedChunk& operator=(const DeformedChunk&) = delete;
			DeformedChunk& operator=(DeformedChunk&&) = delete;

			static Nz::Vector3f DeformPosition(const Nz::Vector3f& position, const Nz::Vector3f& deformationCenter, float deformationRadius);
			static Nz::Quaternionf GetNormalDeformation(const Nz::Vector3f& position, const Nz::Vector3f& faceNormal, const Nz::Vector3f& deformationCenter, float deformationRadius);

		private:
			Nz::Vector3f m_deformationCenter;
			float m_deformationRadius;
	};
}

#include <CommonLib/DeformedChunk.inl>

#endif // TSOM_COMMONLIB_DEFORMEDCHUNK_HPP
