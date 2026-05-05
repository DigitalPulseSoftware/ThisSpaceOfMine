// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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

	enum class NeighborChunk
	{
		// X = -1
		NegX_NegY_NegZ,    // (-1, -1, -1)
		NegX_NegY_ZeroZ,   // (-1, -1,  0)
		NegX_NegY_PosZ,    // (-1, -1,  1)

		NegX_ZeroY_NegZ,   // (-1,  0, -1)
		NegX_ZeroY_ZeroZ,  // (-1,  0,  0)
		NegX_ZeroY_PosZ,   // (-1,  0,  1)

		NegX_PosY_NegZ,    // (-1,  1, -1)
		NegX_PosY_ZeroZ,   // (-1,  1,  0)
		NegX_PosY_PosZ,    // (-1,  1,  1)

		// X = 0
		ZeroX_NegY_NegZ,   // ( 0, -1, -1)
		ZeroX_NegY_ZeroZ,  // ( 0, -1,  0)
		ZeroX_NegY_PosZ,   // ( 0, -1,  1)

		ZeroX_ZeroY_NegZ,  // ( 0,  0, -1)
		ZeroX_ZeroY_ZeroZ, // ( 0,  0,  0) 
		ZeroX_ZeroY_PosZ,  // ( 0,  0,  1)

		ZeroX_PosY_NegZ,   // ( 0,  1, -1)
		ZeroX_PosY_ZeroZ,  // ( 0,  1,  0)
		ZeroX_PosY_PosZ,   // ( 0,  1,  1)

		// X = +1
		PosX_NegY_NegZ,    // ( 1, -1, -1)
		PosX_NegY_ZeroZ,   // ( 1, -1,  0)
		PosX_NegY_PosZ,    // ( 1, -1,  1)

		PosX_ZeroY_NegZ,   // ( 1,  0, -1)
		PosX_ZeroY_ZeroZ,  // ( 1,  0,  0)
		PosX_ZeroY_PosZ,   // ( 1,  0,  1)

		PosX_PosY_NegZ,    // ( 1,  1, -1)
		PosX_PosY_ZeroZ,   // ( 1,  1,  0)
		PosX_PosY_PosZ,    // ( 1,  1,  1)

		Max = PosX_PosY_PosZ
	};

	constexpr bool EnableEnumAsNzFlags(NeighborChunk) { return true; }

	using NeighborChunkMask = Nz::Flags<NeighborChunk>;

	constexpr NeighborChunkMask NeighborChunkMask_All = NeighborChunkMask(NeighborChunkMask::ValueMask);

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

	constexpr Nz::EnumArray<NeighborChunk, Nz::Vector3i32> s_neighborChunkOffset = {
		Nz::Vector3i32{ -1, -1, -1 },
		Nz::Vector3i32{ -1, -1,  0 },
		Nz::Vector3i32{ -1, -1,  1 },
		Nz::Vector3i32{ -1,  0, -1 },
		Nz::Vector3i32{ -1,  0,  0 },
		Nz::Vector3i32{ -1,  0,  1 },
		Nz::Vector3i32{ -1,  1, -1 },
		Nz::Vector3i32{ -1,  1,  0 },
		Nz::Vector3i32{ -1,  1,  1 },

		Nz::Vector3i32{ 0, -1, -1 },
		Nz::Vector3i32{ 0, -1,  0 },
		Nz::Vector3i32{ 0, -1,  1 },
		Nz::Vector3i32{ 0,  0, -1 },
		Nz::Vector3i32{ 0,  0,  0 },
		Nz::Vector3i32{ 0,  0,  1 },
		Nz::Vector3i32{ 0,  1, -1 },
		Nz::Vector3i32{ 0,  1,  0 },
		Nz::Vector3i32{ 0,  1,  1 },

		Nz::Vector3i32{ 1, -1, -1 },
		Nz::Vector3i32{ 1, -1,  0 },
		Nz::Vector3i32{ 1, -1,  1 },
		Nz::Vector3i32{ 1,  0, -1 },
		Nz::Vector3i32{ 1,  0,  0 },
		Nz::Vector3i32{ 1,  0,  1 },
		Nz::Vector3i32{ 1,  1, -1 },
		Nz::Vector3i32{ 1,  1,  0 },
		Nz::Vector3i32{ 1,  1,  1 },
	};

	constexpr Nz::EnumArray<Direction, Direction> s_oppositeDirections = {
		Direction::Front, //< Back
		Direction::Up,    //< Down
		Direction::Back,  //< Front
		Direction::Right, //< Left
		Direction::Left,  //< Right
		Direction::Down,  //< Up
	};

	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal);
	constexpr NeighborChunk ToNeighborChunk(const Nz::Vector3i32& neighborIndices);
}

#include <CommonLib/Direction.inl>

#endif // TSOM_COMMONLIB_DIRECTION_HPP
