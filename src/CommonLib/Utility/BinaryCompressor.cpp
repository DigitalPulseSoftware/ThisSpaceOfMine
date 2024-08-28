// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utility/BinaryCompressor.hpp>
#include <NazaraUtils/Algorithm.hpp>
#include <lz4.h>

namespace tsom
{
	BinaryCompressor::~BinaryCompressor()
	{
		if (m_state)
			LZ4_freeStream(m_state);
	}

	std::optional<std::span<Nz::UInt8>> BinaryCompressor::Compress(const void* data, std::size_t size)
	{
		if (!m_state)
			m_state = LZ4_createStream();

		int dataSize = Nz::SafeCast<int>(size);
		int maxCompressedSize = LZ4_compressBound(dataSize);
		if (maxCompressedSize <= 0)
			return std::nullopt;

		const char* src = static_cast<const char*>(data);

		m_compressedData.resize(maxCompressedSize);
		int compressedSize = LZ4_compress_fast_extState(m_state, src, reinterpret_cast<char*>(m_compressedData.data()), dataSize, maxCompressedSize, 0);
		if (compressedSize <= 0)
			return std::nullopt;

		m_compressedData.resize(compressedSize);
		return m_compressedData;
	}

	std::optional<std::size_t> BinaryCompressor::Decompress(const void* compressedData, std::size_t compressedSize, void* output, std::size_t maxOutputSize)
	{
		const char* src = static_cast<const char*>(compressedData);

		int decompressedSize = LZ4_decompress_safe(src, static_cast<char*>(output), Nz::SafeCast<int>(compressedSize), Nz::SafeCast<int>(maxOutputSize));
		if (decompressedSize < 0)
			return std::nullopt;

		return static_cast<std::size_t>(decompressedSize);
	}

	BinaryCompressor& BinaryCompressor::GetThreadCompressor()
	{
		static thread_local BinaryCompressor binaryCompressor;
		return binaryCompressor;
	}
}
