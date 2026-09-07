// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_RENDERCONSTANTS_HPP
#define TSOM_CLIENTLIB_RENDERCONSTANTS_HPP

#include <Nazara/Math/Angle.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom::Constants
{
	constexpr Nz::DegreeAnglef DefaultCameraFOV = 86.f;

	constexpr Nz::UInt32 RenderMask2D = 0xFFFF0000;
	constexpr Nz::UInt32 RenderMaskUI = 0x00010000;
	constexpr Nz::UInt32 RenderMask3D = 0x0000FFFF;
	constexpr Nz::UInt32 RenderMaskLocalPlayer = 0x00000001;
	constexpr Nz::UInt32 RenderMaskLocalPlayerName = 0x00000002;
	constexpr Nz::UInt32 RenderMaskOtherPlayers = 0x00000004;
	constexpr Nz::UInt32 RenderMaskOtherPlayersName = 0x00000008;
	constexpr Nz::UInt32 RenderMaskPlayerNames = RenderMaskLocalPlayerName | RenderMaskOtherPlayersName;
}

#endif // TSOM_CLIENTLIB_RENDERCONSTANTS_HPP
