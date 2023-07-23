// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_DEFORMEDCHUNK_HPP
#define TSOM_COMMONLIB_DEFORMEDCHUNK_HPP

#include <CommonLib/Chunk.hpp>

namespace tsom
{
	class DeformedChunk : public Chunk
	{
		public:
			inline DeformedChunk(std::size_t width, std::size_t height, std::size_t depth, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius);
			DeformedChunk(const DeformedChunk&) = delete;
			DeformedChunk(DeformedChunk&&) = delete;
			~DeformedChunk() = default;

			std::shared_ptr<Nz::JoltCollider3D> BuildCollider(const Nz::Matrix4f& transformMatrix) const override;
			void BuildMesh(const Nz::Matrix4f& transformMatrix, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices) const override;

			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const override;
			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(std::size_t x, std::size_t y, std::size_t z) const override;

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
