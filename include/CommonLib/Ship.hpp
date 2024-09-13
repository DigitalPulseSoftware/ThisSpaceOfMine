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
			Ship(float tileSize);
			Ship(const Ship&) = delete;
			Ship(Ship&&) = delete;
			~Ship() = default;

			FlatChunk& AddChunk(const BlockLibrary& blockLibrary, const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);

			std::shared_ptr<Nz::Collider3D> BuildHullCollider() const;

			GravityForce ComputeGravity(const Nz::Vector3f& position) const override;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			void Generate(const BlockLibrary& blockLibrary, bool small);

			inline Nz::Vector3f GetCenter() const override;
			inline FlatChunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const FlatChunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;

			void RemoveChunk(const ChunkIndices& indices) override;

			inline void UpdateUpDirection(const Nz::Vector3f& upDirection);

			Ship& operator=(const Ship&) = delete;
			Ship& operator=(Ship&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		private:
			struct ChunkData
			{
				std::shared_ptr<FlatChunk> chunk;

				NazaraSlot(FlatChunk, OnBlockUpdated, onUpdated);
				NazaraSlot(FlatChunk, OnReset, onReset);
			};

			tsl::hopscotch_map<ChunkIndices, ChunkData> m_chunks;
			Nz::Vector3f m_upDirection;
	};
}

#include <CommonLib/Ship.inl>

#endif // TSOM_COMMONLIB_SHIP_HPP
