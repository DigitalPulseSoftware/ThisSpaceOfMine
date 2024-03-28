// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SHIP_HPP
#define TSOM_COMMONLIB_SHIP_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/GravityController.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <tsl/hopscotch_map.h>
#include <memory>
#include <vector>

namespace tsom
{
	class BlockLibrary;

	class TSOM_COMMONLIB_API Ship : public ChunkContainer, public GravityController
	{
		public:
			Ship(const BlockLibrary& blockLibrary, const Nz::Vector3ui& gridSize, float tileSize);
			Ship(const Ship&) = delete;
			Ship(Ship&&) = delete;
			~Ship() = default;

			float ComputeGravityAcceleration(const Nz::Vector3f& position) const override;
			Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const override;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk& GetChunk();
			inline const Chunk& GetChunk() const;
			inline Chunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const Chunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;

			Ship& operator=(const Ship&) = delete;
			Ship& operator=(Ship&&) = delete;

		private:
			void SetupChunk(const BlockLibrary& blockLibrary);

			FlatChunk m_chunk;
	};
}

#include <CommonLib/Ship.inl>

#endif // TSOM_COMMONLIB_SHIP_HPP
