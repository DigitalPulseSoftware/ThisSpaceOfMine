// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <PerlinNoise.hpp>
#include <sol/state.hpp>

namespace tsom
{
	void MathScriptingLibrary::Register(sol::state& state)
	{
		state["DirectionFromNormal"] = &DirectionFromNormal;

		RegisterBox<float>(state, "Boxf");
		RegisterBox<int>(state, "Boxi");
		RegisterBox<unsigned int>(state, "Boxui");
		RegisterEulerAngles<float>(state, "EulerAnglesf");
		RegisterPerlinNoise(state);
		RegisterQuaternion<float>(state, "Quaternionf");
		RegisterVector2<float>(state, "Vec2f");
		RegisterVector2<int>(state, "Vec2i");
		RegisterVector2<unsigned int>(state, "Vec2ui");
		RegisterVector3<float>(state, "Vec3f");
		RegisterVector3<int>(state, "Vec3i");
		RegisterVector3<unsigned int>(state, "Vec3ui");
	}

	template<typename T>
	void MathScriptingLibrary::RegisterBox(sol::state& state, const char* name)
	{
		state.new_usertype<Nz::Box<T>>(name,
			sol::call_constructor, sol::constructors<Nz::Box<T>(), Nz::Box<T>(T, T, T), Nz::Box<T>(const Nz::Vector3<T>& pos, const Nz::Vector3<T>& lengths), Nz::Box<T>(const Nz::Box<T>&)>(),
			"GetCenter", &Nz::Box<T>::GetCenter,
			"GetLengths", &Nz::Box<T>::GetLengths,
			"x", &Nz::Box<T>::x,
			"y", &Nz::Box<T>::y,
			"z", &Nz::Box<T>::z,
			"width", &Nz::Box<T>::width,
			"height", &Nz::Box<T>::height,
			"depth", &Nz::Box<T>::depth,
			sol::meta_function::to_string, &Nz::Box<T>::ToString
		);
	}

	template<typename T>
	void MathScriptingLibrary::RegisterEulerAngles(sol::state& state, const char* name)
	{
		state.new_usertype<Nz::EulerAngles<T>>(name,
			sol::call_constructor, sol::constructors<Nz::EulerAngles<T>(), Nz::EulerAngles<T>(T, T, T), Nz::EulerAngles<T>(const Nz::EulerAngles<T>&)>(),
			"pitch", &Nz::EulerAngles<T>::pitch,
			"yaw", &Nz::EulerAngles<T>::yaw,
			"roll", &Nz::EulerAngles<T>::roll,
			sol::meta_function::to_string, &Nz::EulerAngles<T>::ToString
		);
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

	template<typename T>
	void MathScriptingLibrary::RegisterQuaternion(sol::state& state, const char* name)
	{
		state.new_usertype<Nz::Quaternion<T>>(name,
			sol::call_constructor, sol::constructors<Nz::Quaternion<T>(), Nz::Quaternion<T>(T, T, T, T), Nz::Quaternion<T>(const Nz::Quaternion<T>&)>(),
			"GetConjugate", &Nz::Quaternion<T>::GetConjugate,
			"x", &Nz::Quaternion<T>::x,
			"y", &Nz::Quaternion<T>::y,
			"z", &Nz::Quaternion<T>::z,
			"w", &Nz::Quaternion<T>::w,
			sol::meta_function::multiplication, sol::overload(Nz::Overload<const Nz::Quaternion<T>&>(&Nz::Quaternion<T>::operator*), Nz::Overload<const Nz::Vector3<T>&>(&Nz::Quaternion<T>::operator*)),
			sol::meta_function::to_string, &Nz::Quaternion<T>::ToString
		);
	}

	template<typename T>
	void MathScriptingLibrary::RegisterVector2(sol::state& state, const char* name)
	{
		state.new_usertype<Nz::Vector2<T>>(name,
			sol::call_constructor, sol::constructors<Nz::Vector2<T>(), Nz::Vector2<T>(T), Nz::Vector2<T>(T, T), Nz::Vector2<T>(const Nz::Vector2<T>&)>(),
			"GetLength", [](const Nz::Vector2<T>& vec)
			{
				return vec.template GetLength<lua_Number>();
			},
			"GetNormal", [](const Nz::Vector2<T>& vec)
			{
				T length;
				Nz::Vector2<T> normalizedVec = vec.GetNormal(&length);

				return std::make_pair(normalizedVec, length);
			},
			"GetSquaredLength", &Nz::Vector2<T>::GetSquaredLength,
			"Distance", [](const Nz::Vector2<T>& vec1, const Nz::Vector2<T>& vec2)
			{
				return vec1.Distance(vec2);
			},
			"SquaredDistance", [](const Nz::Vector2<T>& vec1, const Nz::Vector2<T>& vec2)
			{
				return vec1.SquaredDistance(vec2);
			},
			"x", &Nz::Vector2<T>::x,
			"y", &Nz::Vector2<T>::y,
			sol::meta_function::addition, Nz::Overload<const Nz::Vector2<T>&>(&Nz::Vector2<T>::operator+),
			sol::meta_function::division, sol::overload(Nz::Overload<T>(&Nz::Vector2<T>::operator/), Nz::Overload<const Nz::Vector2<T>&>(&Nz::Vector2<T>::operator/)),
			sol::meta_function::multiplication, sol::overload(Nz::Overload<T>(&Nz::Vector2<T>::operator*), Nz::Overload<const Nz::Vector2<T>&>(&Nz::Vector2<T>::operator*)),
			sol::meta_function::subtraction, Nz::Overload<const Nz::Vector2<T>&>(&Nz::Vector2<T>::operator-),
			sol::meta_function::to_string, &Nz::Vector2<T>::ToString,
			sol::meta_function::unary_minus, Nz::Overload<>(&Nz::Vector2<T>::operator-)
		);
	}

	template<typename T>
	void MathScriptingLibrary::RegisterVector3(sol::state& state, const char* name)
	{
		state.new_usertype<Nz::Vector3<T>>(name,
			sol::call_constructor, sol::constructors<Nz::Vector3<T>(), Nz::Vector3<T>(T), Nz::Vector3<T>(T, T, T), Nz::Vector3<T>(const Nz::Vector3<T>&)>(),
			"GetLength", [](const Nz::Vector3<T>& vec)
			{
				return vec.template GetLength<lua_Number>();
			},
			"GetNormal", [](const Nz::Vector3<T>& vec)
			{
				T length;
				Nz::Vector3<T> normalizedVec = vec.GetNormal(&length);

				return std::make_pair(normalizedVec, length);
			},
			"GetSquaredLength", &Nz::Vector3<T>::GetSquaredLength,
			"Distance", [](const Nz::Vector3<T>& vec1, const Nz::Vector3<T>& vec2)
			{
				return vec1.Distance(vec2);
			},
			"SquaredDistance", [](const Nz::Vector3<T>& vec1, const Nz::Vector3<T>& vec2)
			{
				return vec1.SquaredDistance(vec2);
			},
			"x", &Nz::Vector3<T>::x,
			"y", &Nz::Vector3<T>::y,
			"z", &Nz::Vector3<T>::z,
			sol::meta_function::addition, Nz::Overload<const Nz::Vector3<T>&>(&Nz::Vector3<T>::operator+),
			sol::meta_function::division, sol::overload(Nz::Overload<T>(&Nz::Vector3<T>::operator/), Nz::Overload<const Nz::Vector3<T>&>(&Nz::Vector3<T>::operator/)),
			sol::meta_function::multiplication, sol::overload(Nz::Overload<T>(&Nz::Vector3<T>::operator*), Nz::Overload<const Nz::Vector3<T>&>(&Nz::Vector3<T>::operator*)),
			sol::meta_function::subtraction, Nz::Overload<const Nz::Vector3<T>&>(&Nz::Vector3<T>::operator-),
			sol::meta_function::to_string, &Nz::Vector3<T>::ToString,
			sol::meta_function::unary_minus, Nz::Overload<>(&Nz::Vector3<T>::operator-)
		);
	}
}
