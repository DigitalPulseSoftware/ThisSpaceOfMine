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

			std::shared_ptr<Nz::Collider3D> BuildCollider() const override;

			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const override;
			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(const Nz::Vector3ui& indices) const override;

			Nz::Vector3f DeformPosition(const Nz::Vector3f& position) const;

			inline void UpdateDeformationRadius(float deformationRadius);

			DeformedChunk& operator=(const DeformedChunk&) = delete;
			DeformedChunk& operator=(DeformedChunk&&) = delete;

		private:
			Nz::Vector3f m_deformationCenter;
			float m_deformationRadius;
	};
}

#include <CommonLib/DeformedChunk.inl>

#endif // TSOM_COMMONLIB_DEFORMEDCHUNK_HPP
