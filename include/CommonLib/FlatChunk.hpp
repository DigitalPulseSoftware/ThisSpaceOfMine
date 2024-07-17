// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_FLATCHUNK_HPP
#define TSOM_COMMONLIB_FLATCHUNK_HPP

#include <CommonLib/Chunk.hpp>

namespace tsom
{
	class FlatChunk : public Chunk
	{
		public:
			using Chunk::Chunk;
			FlatChunk(const FlatChunk&) = delete;
			FlatChunk(FlatChunk&&) = delete;
			~FlatChunk() = default;

			std::shared_ptr<Nz::Collider3D> BuildCollider(const BlockLibrary& blockManager) const override;
			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const override;
			Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(const Nz::Vector3ui& indices) const override;

			FlatChunk& operator=(const FlatChunk&) = delete;
			FlatChunk& operator=(FlatChunk&&) = delete;
	};
}

#include <CommonLib/FlatChunk.inl>

#endif // TSOM_COMMONLIB_FLATCHUNK_HPP
