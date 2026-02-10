// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Constants.hpp>

namespace tsom
{
	template<bool Write>
	ChunkLock<Write>::ChunkLock(ChunkType chunk) :
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
		if constexpr (Write)
			m_chunk->LockWrite();
		else
			m_chunk->LockRead();

		m_isLocked = true;
	}

	template<bool Write>
	void ChunkLock<Write>::Unlock()
	{
		NazaraAssertMsg(m_isLocked, "ChunkLock isn't locked");
		if constexpr (Write)
			m_chunk->UnlockWrite();
		else
			m_chunk->UnlockRead();

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
				if (i != lastFailureMutex && !m_chunks[i]->TryLockRead())
				{
					succeeded = false;

					// Lock failed, unlock everything and try again
					for (std::size_t j = 0; j < i; ++j)
						m_chunks[j]->UnlockRead();

					// Lock blocked chunk first to pause thread
					m_chunks[i]->LockRead();
					lastFailureMutex = i;
					break;
				}
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
			chunk->UnlockRead();

		m_isLocked = false;
	}
}
