// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SURFACENETSCHUNK_HPP
#define TSOM_COMMONLIB_SURFACENETSCHUNK_HPP

#include <CommonLib/Chunk.hpp>
#include <Nazara/Math/Box.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <array>

namespace tsom
{
	class TSOM_COMMONLIB_API SurfaceNetsChunk : public Chunk
	{
		public:
			using Chunk::Chunk;
			SurfaceNetsChunk(const SurfaceNetsChunk&) = delete;
			SurfaceNetsChunk(SurfaceNetsChunk&&) = delete;
			~SurfaceNetsChunk() = default;

			std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale = 1.f) const override;
			std::shared_ptr<Nz::Collider3D> BuildCollider(std::size_t layerIndex) const override;
			void BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& center, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace) const override;

			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeBlockCorners(const Nz::Vector3ui& indices) const override;
			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const;
			std::optional<HitBlock> ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const override;

			SurfaceNetsChunk& operator=(const SurfaceNetsChunk&) = delete;
			SurfaceNetsChunk& operator=(SurfaceNetsChunk&&) = delete;

		private:
			using NeighborChunkArray = std::array<const Chunk*, 27>;

			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> BuildCorners(const Nz::Vector3ui& indices, const NeighborChunkArray& neighborChunks) const;
			void BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace, bool generateVisualMesh) const;

			static inline std::size_t GetNeighborIndex(const ChunkIndices& chunkIndices);

			BlockIndex GetNeighborBlock(const NeighborChunkArray& neighborChunks, Nz::Vector3ui indices, const Nz::Vector3i& offset) const;
	};
}

#include <CommonLib/SurfaceNetsChunk.inl>

#endif // TSOM_COMMONLIB_SURFACENETSCHUNK_HPP
