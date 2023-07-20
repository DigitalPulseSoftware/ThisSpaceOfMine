// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_DIRECTION_HPP
#define TSOM_COMMONLIB_UTILITY_DIRECTION_HPP

#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Vector3.hpp>

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

	constexpr Nz::EnumArray<tsom::Direction, Nz::Vector3f> s_dirNormals = {
		Nz::Vector3f::Backward(),
		Nz::Vector3f::Down(),
		Nz::Vector3f::Forward(),
		Nz::Vector3f::Left(),
		Nz::Vector3f::Right(),
		Nz::Vector3f::Up()
	};

	// Debug colors
	constexpr Nz::EnumArray<tsom::Direction, Nz::Color> s_dirColors = {
		Nz::Color(0.9f, 0.9f, 0.9f), //< Back
		Nz::Color(1.f, 0.9f, 1.f),   //< Down
		Nz::Color(0.9f, 0.9f, 1.f),  //< Forward
		Nz::Color(1.f, 0.9f, 0.9f),  //< Left
		Nz::Color(1.f, 1.f, 0.9f),   //< Right
		Nz::Color(0.9f, 1.f, 0.9f),  //< Up
	};

	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal);
}

#include <CommonLib/Direction.inl>

#endif // TSOM_COMMONLIB_UTILITY_DIRECTION_HPP
