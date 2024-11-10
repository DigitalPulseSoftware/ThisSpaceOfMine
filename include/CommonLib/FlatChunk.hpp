// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_FLATCHUNK_HPP
#define TSOM_COMMONLIB_FLATCHUNK_HPP

#include <CommonLib/Chunk.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <Nazara/Math/Box.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API FlatChunk : public Chunk
	{
		public:
			using Chunk::Chunk;
			FlatChunk(const FlatChunk&) = delete;
			FlatChunk(FlatChunk&&) = delete;
			~FlatChunk() = default;

			std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale = 1.f) const override;
			std::shared_ptr<Nz::Collider3D> BuildCollider() const override;

			std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const;
			std::optional<HitBlock> ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const override;

			FlatChunk& operator=(const FlatChunk&) = delete;
			FlatChunk& operator=(FlatChunk&&) = delete;

			static void BuildCollider(const Nz::Vector3ui& dims, Nz::Bitset<Nz::UInt64> collisionCellMask, Nz::FunctionRef<void(const Nz::Boxf& box)> callback);
	};
}

#include <CommonLib/FlatChunk.inl>

#endif // TSOM_COMMONLIB_FLATCHUNK_HPP
