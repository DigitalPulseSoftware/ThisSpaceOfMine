// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DIRECTION_HPP
#define TSOM_COMMONLIB_DIRECTION_HPP

#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/Constants.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/Flags.hpp>

namespace tsom
{
	enum class Direction
	{
		Back,
		Down,
		Front,
		Left,
		Right,
		Up,

		Max = Up
	};

	constexpr bool EnableEnumAsNzFlags(Direction) { return true; }

	using DirectionMask = Nz::Flags<Direction>;

	constexpr DirectionMask DirectionMask_All = DirectionMask(DirectionMask::ValueMask);

	constexpr Nz::EnumArray<Direction, Nz::Vector3f> s_dirNormals = {
		Nz::Vector3f::Backward(),
		Nz::Vector3f::Down(),
		Nz::Vector3f::Forward(),
		Nz::Vector3f::Left(),
		Nz::Vector3f::Right(),
		Nz::Vector3f::Up()
	};

	// Debug colors
	constexpr Nz::EnumArray<Direction, Nz::Color> s_dirColors = {
		Nz::Color::Green(), //< Back
		Nz::Color::Gray(),  //< Down
		Nz::Color::White(), //< Front
		Nz::Color::Red(),   //< Left
		Nz::Color::Blue(),  //< Right
		Nz::Color::Cyan(),  //< Up
	};

	struct DirectionAxis
	{
		unsigned int forwardAxis;
		unsigned int rightAxis;
		unsigned int upAxis;
		int forwardDir;
		int rightDir;
		int upDir;
	};

	constexpr Nz::EnumArray<Direction, DirectionAxis> s_dirAxis = {
		DirectionAxis { 1, 0, 2,  1,  1,  1 }, //< Back
		DirectionAxis { 2, 0, 1,  1,  1, -1 }, //< Down
		DirectionAxis { 1, 0, 2, -1,  1, -1 }, //< Front
		DirectionAxis { 2, 1, 0, -1,  1, -1 }, //< Left
		DirectionAxis { 2, 1, 0, -1, -1,  1 }, //< Right
		DirectionAxis { 2, 0, 1, -1,  1,  1 }, //< Up
	};

	constexpr Nz::EnumArray<Direction, Nz::Vector3i> s_blockDirOffset = {
		Nz::Vector3i {  0,  1,  0 }, //< Back
		Nz::Vector3i {  0,  0, -1 }, //< Down
		Nz::Vector3i {  0, -1,  0 }, //< Front
		Nz::Vector3i { -1,  0,  0 }, //< Left
		Nz::Vector3i {  1,  0,  0 }, //< Right
		Nz::Vector3i {  0,  0,  1 }, //< Up
	};

	constexpr Nz::EnumArray<Direction, Nz::Vector3i> s_chunkDirOffset = {
		Nz::Vector3i {  0,  0,  1 }, //< Back
		Nz::Vector3i {  0, -1,  0 }, //< Down
		Nz::Vector3i {  0,  0, -1 }, //< Front
		Nz::Vector3i { -1,  0,  0 }, //< Left
		Nz::Vector3i {  1,  0,  0 }, //< Right
		Nz::Vector3i {  0,  1,  0 }, //< Up
	};

	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal);
}

#include <CommonLib/Direction.inl>

#endif // TSOM_COMMONLIB_DIRECTION_HPP
