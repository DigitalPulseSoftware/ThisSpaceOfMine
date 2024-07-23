// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	constexpr std::optional<PlayerPermission> PlayerPermissionFromString(std::string_view permissionStr)
	{
		for (std::size_t i = 0; i <= Nz::UnderlyingCast(PlayerPermission::Max); ++i)
		{
			PlayerPermission permission = static_cast<PlayerPermission>(i);
			if (permissionStr == ToString(permission))
				return permission;
		}

		return {};
	}

	constexpr std::string_view ToString(PlayerPermission permission)
	{
		switch (permission)
		{
			case PlayerPermission::Admin: return "admin";
		}

		NAZARA_UNREACHABLE();
	}
}
