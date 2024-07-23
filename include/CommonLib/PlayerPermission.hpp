// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLAYERPERMISSION_HPP
#define TSOM_COMMONLIB_PLAYERPERMISSION_HPP

#include <NazaraUtils/Algorithm.hpp>
#include <NazaraUtils/Flags.hpp>
#include <optional>
#include <string_view>

namespace tsom
{
	enum class PlayerPermission
	{
		Admin,

		Max = Admin
	};

	constexpr bool EnableEnumAsNzFlags(PlayerPermission) { return true; }

	constexpr std::optional<PlayerPermission> PlayerPermissionFromString(std::string_view permissionStr);

	constexpr std::string_view ToString(PlayerPermission permission);

	using PlayerPermissionFlags = Nz::Flags<PlayerPermission>;
}

#include <CommonLib/PlayerPermission.inl>

#endif // TSOM_COMMONLIB_PLAYERPERMISSION_HPP
