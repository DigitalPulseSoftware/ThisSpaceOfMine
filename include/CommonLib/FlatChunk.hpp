// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_FLATCHUNK_HPP
#define TSOM_COMMONLIB_FLATCHUNK_HPP

#include <CommonLib/Chunk.hpp>
#include <NazaraUtils/FunctionRef.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API FlatChunk : public Chunk
	{
		public:
			using Chunk::Chunk;
			FlatChunk(const FlatChunk&) = delete;
			FlatChunk(FlatChunk&&) = delete;
			~FlatChunk() = default;

			std::shared_ptr<Nz::Collider3D> BuildCollider() const override;
			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const override;

			FlatChunk& operator=(const FlatChunk&) = delete;
			FlatChunk& operator=(FlatChunk&&) = delete;

			static void BuildCollider(const Nz::Vector3ui& dims, Nz::Bitset<Nz::UInt64> collisionCellMask, Nz::FunctionRef<void(const Nz::Boxf& box)> callback);
	};
}

#include <CommonLib/FlatChunk.inl>

#endif // TSOM_COMMONLIB_FLATCHUNK_HPP
