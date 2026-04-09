// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Constants.hpp>

namespace tsom
{
	namespace Detail
	{
		template<bool Write, typename T>
		void LockChunk(T* chunk)
		{
			if constexpr (Write)
				chunk->LockWrite();
			else
				chunk->LockRead();
		}

		template<bool Write, typename T>
		bool TryLockChunk(T* chunk)
		{
			if constexpr (Write)
				return chunk->TryLockWrite();
			else
				return chunk->TryLockRead();
		}

		template<bool Write, typename T>
		void UnlockChunk(T* chunk)
		{
			if constexpr (Write)
				chunk->UnlockWrite();
			else
				chunk->UnlockRead();
		}
	}

	template<bool Write>
	ChunkLock<Write>::ChunkLock(ChunkType chunk) :
	m_chunk(chunk),
	m_isLocked(false)
	{
		Lock();
	}

	template<bool Write>
	ChunkLock<Write>::ChunkLock(ChunkType chunk, std::adopt_lock_t) :
	m_chunk(chunk),
	m_isLocked(true)
	{
	}

	template<bool Write>
	ChunkLock<Write>::ChunkLock(ChunkType chunk, std::defer_lock_t) :
	m_chunk(chunk),
	m_isLocked(false)
	{
	}

	template<bool Write>
	ChunkLock<Write>::ChunkLock(ChunkLock&& chunkLock) noexcept :
	m_chunk(std::exchange(chunkLock.m_chunk, nullptr)),
	m_isLocked(std::exchange(chunkLock.m_isLocked, false))
	{
	}

	template<bool Write>
	ChunkLock<Write>::~ChunkLock()
	{
		if (m_isLocked)
			Unlock();
	}

	template<bool Write>
	void ChunkLock<Write>::Lock()
	{
		NazaraAssertMsg(!m_isLocked, "ChunkLock is already locked");

		Detail::LockChunk<Write>(m_chunk);
		m_isLocked = true;
	}

	template<bool Write>
	bool ChunkLock<Write>::TryLock()
	{
		NazaraAssertMsg(!m_isLocked, "ChunkLock is already locked");

		if (Detail::TryLockChunk<Write>(m_chunk))
		{
			m_isLocked = true;
			return true;
		}

		return false;
	}

	template<bool Write>
	void ChunkLock<Write>::Unlock()
	{
		NazaraAssertMsg(m_isLocked, "ChunkLock isn't locked");

		Detail::UnlockChunk<Write>(m_chunk);
		m_isLocked = false;
	}


	template<bool Write, std::size_t Max>
	MultiChunkLock<Write, Max>::MultiChunkLock() :
	m_isLocked(false)
	{
	}

	template<bool Write, std::size_t Max>
	MultiChunkLock<Write, Max>::MultiChunkLock(MultiChunkLock&& chunkLock) noexcept :
	m_chunks(std::move(chunkLock.m_chunks)),
	m_isLocked(std::exchange(chunkLock.m_isLocked, false))
	{
	}

	template<bool Write, std::size_t Max>
	MultiChunkLock<Write, Max>::~MultiChunkLock()
	{
		if (m_isLocked)
			Unlock();
	}

	template<bool Write, std::size_t Max>
	void MultiChunkLock<Write, Max>::AddChunk(ChunkType chunk)
	{
		NazaraAssertMsg(!m_isLocked, "ChunkLock is already locked");
		NazaraAssertMsg(std::find(m_chunks.begin(), m_chunks.end(), chunk) == m_chunks.end(), "Chunk is already in the list");
		m_chunks.push_back(chunk);
	}

	template<bool Write, std::size_t Max>
	void MultiChunkLock<Write, Max>::Lock()
	{
		// Algorithm to lock every chunk without deadlock
		std::size_t lastFailureMutex = Nz::MaxValue();

		for (;;)
		{
			bool succeeded = true;
			for (std::size_t i = 0; i < m_chunks.size(); ++i)
			{
				if (i == lastFailureMutex)
					continue;

				if (Detail::TryLockChunk<Write>(m_chunks[i]))
					continue;

				// Lock failed, unlock everything and try again
				succeeded = false;

				for (std::size_t j = 0; j < i; ++j)
					Detail::UnlockChunk<Write>(m_chunks[j]);

				if (lastFailureMutex > i && lastFailureMutex < m_chunks.size())
					Detail::UnlockChunk<Write>(m_chunks[lastFailureMutex]);

				// Lock blocked chunk first to pause thread
				Detail::LockChunk<Write>(m_chunks[i]);

				lastFailureMutex = i;
				break;
			}

			if (succeeded)
				break;
		}

		m_isLocked = true;
	}

	template<bool Write, std::size_t Max>
	void MultiChunkLock<Write, Max>::Unlock()
	{
		NazaraAssertMsg(m_isLocked, "ChunkLock isn't locked");
		for (const Chunk* chunk : m_chunks)
			Detail::UnlockChunk<Write>(chunk);

		m_isLocked = false;
	}
}
