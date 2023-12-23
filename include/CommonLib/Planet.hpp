// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_PLANET_HPP
#define TSOM_COMMONLIB_PLANET_HPP

#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Export.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API Planet : public ChunkContainer
	{
		public:
			Planet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius, float gravityFactor);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			Chunk& AddChunk(const Nz::Vector3ui& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);

			Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const;

			void GenerateChunks(BlockLibrary& blockLibrary);
			void GeneratePlatform(BlockLibrary& blockLibrary, Direction upDirection, const Nz::Vector3ui& platformCenter);

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(std::size_t chunkIndex) override;
			inline const Chunk* GetChunk(std::size_t chunkIndex) const override;
			inline Chunk& GetChunk(const Nz::Vector3ui& indices) override;
			inline const Chunk& GetChunk(const Nz::Vector3ui& indices) const override;
			inline std::size_t GetChunkCount() const override;
			inline float GetCornerRadius() const;
			inline float GetGravityFactor(const Nz::Vector3f& position) const;

			void RemoveChunk(const Nz::Vector3ui& indices);

			inline void UpdateCornerRadius(float cornerRadius);

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		protected:
			struct ChunkData
			{
				std::unique_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
			};

			std::vector<ChunkData> m_chunks;
			float m_cornerRadius;
			float m_gravityFactor;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
