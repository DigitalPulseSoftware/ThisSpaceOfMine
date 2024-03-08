// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utils.hpp>
#include <fmt/format.h>
#include <array>
#include <cassert>

namespace tsom
{
	std::string ByteToString(Nz::UInt64 bytes, bool speed)
	{
		constexpr std::array<std::string_view, 9> s_suffixes = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };

		std::size_t suffixIndex = 0;
		Nz::UInt64 rem = 0;
		while (bytes > 1024 && suffixIndex < s_suffixes.size() - 1)
		{
			rem = bytes % 1024;
			bytes /= 1024;
			suffixIndex++;
		}

		assert(suffixIndex < s_suffixes.size());
		std::string str = std::to_string(bytes);
		if (rem > 0)
			return fmt::format("{0}.{1} {2}{3}", bytes, 1000 * rem / 1024, s_suffixes[suffixIndex], (speed) ? "/s" : "");
		else
			return fmt::format("{0} {1}{2}", bytes, s_suffixes[suffixIndex], (speed) ? "/s" : "");
	}
}
