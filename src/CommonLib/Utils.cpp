// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utils.hpp>
#include <fmt/format.h>
#include <array>

namespace tsom
{
	std::string FormatSize(Nz::UInt64 sizeInBytes)
	{
		constexpr std::array<std::string_view, 7> s_units = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

		std::size_t unitIndex = 0;
		for (; unitIndex < s_units.size(); ++unitIndex)
		{
			if (sizeInBytes < 1024 * 1024)
				break;

			sizeInBytes /= 1024;
		}

		double size = 0.0;
		if (sizeInBytes > 1024 && unitIndex < s_units.size() - 1)
		{
			size = sizeInBytes / 1024.0;
			unitIndex++;
		}
		else
			size = sizeInBytes;

		return fmt::format("{:.2f} {}", size, s_units[unitIndex]);
	}
}
