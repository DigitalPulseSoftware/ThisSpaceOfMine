// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SHIP_HPP
#define TSOM_COMMONLIB_SHIP_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Direction.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class BlockLibrary;

	class TSOM_COMMONLIB_API Ship : public ChunkContainer
	{
		public:
			Ship(const BlockLibrary& blockLibrary, const Nz::Vector3ui& gridSize, float tileSize);
			Ship(const Ship&) = delete;
			Ship(Ship&&) = delete;
			~Ship() = default;

			Chunk& AddChunk(const Nz::Vector3ui& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr);

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(std::size_t chunkIndex) override;
			inline const Chunk* GetChunk(std::size_t chunkIndex) const override;
			inline Chunk& GetChunk(const Nz::Vector3ui& indices) override;
			inline const Chunk& GetChunk(const Nz::Vector3ui& indices) const override;
			inline Nz::Vector3ui GetChunkIndices(std::size_t chunkIndex) const;
			inline std::size_t GetChunkCount() const override;

			void RemoveChunk(const Nz::Vector3ui& indices);

			Ship& operator=(const Ship&) = delete;
			Ship& operator=(Ship&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		private:
			void SetupChunks(const BlockLibrary& blockLibrary);

			struct ChunkData
			{
				std::unique_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
			};

			std::vector<ChunkData> m_chunks;
	};
}

#include <CommonLib/Ship.inl>

#endif // TSOM_COMMONLIB_SHIP_HPP
