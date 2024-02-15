// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline bool IsInputMoreRecent(InputIndex a, InputIndex b)
	{
		static constexpr InputIndex half = std::numeric_limits<InputIndex>::max() / 2;

		if (a > b)
		{
			if (a - b < half)
				return true;
		}
		else if (b > a)
		{
			if (b - a > half)
				return true;
		}

		return false;
	}
}
