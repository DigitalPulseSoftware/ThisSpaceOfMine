// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKLOCK_HPP
#define TSOM_COMMONLIB_CHUNKLOCK_HPP

#include <CommonLib/Chunk.hpp>
#include <CommonLib/Export.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <utility>

namespace tsom
{
	template<bool Write>
	class ChunkLock
	{
		using ChunkType = std::conditional_t<Write, Chunk*, const Chunk*>;

		public:
			ChunkLock(ChunkType chunk);
			ChunkLock(const ChunkLock&) = delete;
			ChunkLock(ChunkLock&& chunkLock) noexcept;
			~ChunkLock();

			void Lock();
			void Unlock();

			ChunkLock& operator=(const ChunkLock&) = delete;
			ChunkLock& operator=(ChunkLock&&) = delete;

		private:
			ChunkType m_chunk;
			bool m_isLocked;
	};
	
	template<bool Write, std::size_t Max = 3 * 3 * 3>
	class MultiChunkLock
	{
		using ChunkType = std::conditional_t<Write, Chunk*, const Chunk*>;

		public:
			MultiChunkLock();
			MultiChunkLock(const MultiChunkLock&) = delete;
			MultiChunkLock(MultiChunkLock&& chunkLock) noexcept;
			~MultiChunkLock();

			void AddChunk(ChunkType chunk);

			void Lock();
			void Unlock();

			MultiChunkLock& operator=(const MultiChunkLock&) = delete;
			MultiChunkLock& operator=(MultiChunkLock&&) = delete;

		private:
			Nz::HybridVector<ChunkType, Max> m_chunks;
			bool m_isLocked;
	};

	using ChunkReadLock = ChunkLock<false>;
	using ChunkWriteLock = ChunkLock<true>;

	using MultiChunkReadLock = MultiChunkLock<false>;
	using MultiChunkWriteLock = MultiChunkLock<true>;
}

#include <CommonLib/ChunkLock.inl>

#endif // TSOM_COMMONLIB_CHUNKLOCK_HPP
