// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <PerlinNoise.hpp>
#include <fmt/format.h>
#include <sol/state.hpp>

namespace tsom
{
	void MathScriptingLibrary::Register(sol::state& state)
	{
		state["DirectionFromNormal"] = &DirectionFromNormal;
		RegisterPerlinNoise(state);
	}

	void MathScriptingLibrary::RegisterPerlinNoise(sol::state& state)
	{
		state.new_usertype<siv::PerlinNoise>("PerlinNoise",
			sol::call_constructor, sol::constructors<siv::PerlinNoise(), siv::PerlinNoise(std::uint32_t)>(),
			"noise1D", LuaFunction(&siv::PerlinNoise::noise1D),
			"noise1D_01", LuaFunction(&siv::PerlinNoise::noise1D_01),
			"noise2D", LuaFunction(&siv::PerlinNoise::noise2D),
			"noise2D_01", LuaFunction(&siv::PerlinNoise::noise2D_01),
			"noise3D", LuaFunction(&siv::PerlinNoise::noise3D),
			"noise3D_01", LuaFunction(&siv::PerlinNoise::noise3D_01),

			"octave1D", LuaFunction(&siv::PerlinNoise::octave1D),
			"octave2D", LuaFunction(&siv::PerlinNoise::octave2D),
			"octave3D", LuaFunction(&siv::PerlinNoise::octave3D),

			"octave1D_11", LuaFunction(&siv::PerlinNoise::octave1D_11),
			"octave2D_11", LuaFunction(&siv::PerlinNoise::octave2D_11),
			"octave3D_11", LuaFunction(&siv::PerlinNoise::octave3D_11),

			"octave1D_01", LuaFunction(&siv::PerlinNoise::octave1D_01),
			"octave2D_01", LuaFunction(&siv::PerlinNoise::octave2D_01),
			"octave3D_01", LuaFunction(&siv::PerlinNoise::octave3D_01),

			"reseed", LuaFunction([](siv::PerlinNoise& noise, unsigned int seed)
			{
				return noise.reseed(seed);
			}),//LuaFunction(Nz::Overload<unsigned int>(&siv::PerlinNoise::reseed)),

			"normalizedOctave1D", LuaFunction(&siv::PerlinNoise::normalizedOctave1D),
			"normalizedOctave2D", LuaFunction(&siv::PerlinNoise::normalizedOctave2D),
			"normalizedOctave3D", LuaFunction(&siv::PerlinNoise::normalizedOctave3D),

			"normalizedOctave1D_01", LuaFunction(&siv::PerlinNoise::normalizedOctave1D_01),
			"normalizedOctave2D_01", LuaFunction(&siv::PerlinNoise::normalizedOctave2D_01),
			"normalizedOctave3D_01", LuaFunction(&siv::PerlinNoise::normalizedOctave3D_01)
		);
	}
}
