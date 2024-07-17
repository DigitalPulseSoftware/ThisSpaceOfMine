// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DIRECTION_HPP
#define TSOM_COMMONLIB_DIRECTION_HPP

#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/EnumArray.hpp>

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

	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal);
}

#include <CommonLib/Direction.inl>

#endif // TSOM_COMMONLIB_DIRECTION_HPP
