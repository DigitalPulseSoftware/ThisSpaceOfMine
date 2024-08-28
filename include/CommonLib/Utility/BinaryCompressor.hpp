// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_BINARYCOMPRESSOR_HPP
#define TSOM_COMMONLIB_UTILITY_BINARYCOMPRESSOR_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/MovablePtr.hpp>
#include <optional>
#include <span>
#include <vector>

typedef union LZ4_stream_u LZ4_stream_t;

namespace tsom
{
	class TSOM_COMMONLIB_API BinaryCompressor
	{
		public:
			BinaryCompressor() = default;
			BinaryCompressor(const BinaryCompressor&) = delete;
			BinaryCompressor(BinaryCompressor&&) noexcept = default;
			~BinaryCompressor();

			std::optional<std::span<Nz::UInt8>> Compress(const void* data, std::size_t size);
			std::optional<std::size_t> Decompress(const void* compressedData, std::size_t compressedSize, void* output, std::size_t maxOutputSize);

			BinaryCompressor& operator=(const BinaryCompressor&) = delete;
			BinaryCompressor& operator=(BinaryCompressor&&) noexcept = default;

			static BinaryCompressor& GetThreadCompressor();

		private:
			std::vector<Nz::UInt8> m_compressedData;
			Nz::MovablePtr<LZ4_stream_t> m_state;
	};
}

#include <CommonLib/Utility/BinaryCompressor.inl>

#endif // TSOM_COMMONLIB_UTILITY_BINARYCOMPRESSOR_HPP
