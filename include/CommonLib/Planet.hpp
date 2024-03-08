// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLANET_HPP
#define TSOM_COMMONLIB_PLANET_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Direction.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <tsl/hopscotch_map.h>
#include <memory>
#include <vector>

namespace Nz
{
	class TaskScheduler;
}

namespace tsom
{
	class TSOM_COMMONLIB_API Planet : public ChunkContainer
	{
		public:
			Planet(float tileSize, float cornerRadius, float gravityFactor);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			Chunk& AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);

			Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			void GenerateChunk(const BlockLibrary& blockLibrary, Chunk& chunk, unsigned int seed, const Nz::Vector3ui& chunkCount);
			void GenerateChunks(const BlockLibrary& blockLibrary, Nz::TaskScheduler& taskScheduler, unsigned int seed, const Nz::Vector3ui& chunkCount);
			void GeneratePlatform(const BlockLibrary& blockLibrary, Direction upDirection, const BlockIndices& platformCenter);

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const Chunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;
			inline float GetCornerRadius() const;
			inline float GetGravityFactor(const Nz::Vector3f& position) const;

			void RemoveChunk(const ChunkIndices& indices);

			inline void UpdateCornerRadius(float cornerRadius);

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		protected:
			struct ChunkData
			{
				std::unique_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
				NazaraSlot(Chunk, OnReset, onReset);
			};

			tsl::hopscotch_map<ChunkIndices, ChunkData> m_chunks;
			float m_cornerRadius;
			float m_gravityFactor;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_PLANET_HPP
