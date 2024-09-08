// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <sol/state.hpp>

namespace tsom
{
	void MathScriptingLibrary::Register(sol::state& state)
	{
		RegisterBox(state);
		RegisterEulerAngles(state);
		RegisterVector2(state);
		RegisterVector3(state);
	}

	void MathScriptingLibrary::RegisterBox(sol::state& state)
	{
		state.new_usertype<Nz::Boxf>("Box",
			sol::call_constructor, sol::constructors<Nz::Boxf(), Nz::Boxf(float, float, float), Nz::Boxf(const Nz::Vector3f& pos, const Nz::Vector3f& lengths)>(),
			"GetLengths", &Nz::Boxf::GetLengths,
			"x", &Nz::Boxf::x,
			"y", &Nz::Boxf::y,
			"z", &Nz::Boxf::z,
			"width", &Nz::Boxf::width,
			"height", &Nz::Boxf::height,
			"depth", &Nz::Boxf::depth,
			sol::meta_function::to_string, &Nz::Boxf::ToString
		);
	}

	void MathScriptingLibrary::RegisterEulerAngles(sol::state& state)
	{
		state.new_usertype<Nz::EulerAnglesf>("EulerAngles",
			sol::call_constructor, sol::constructors<Nz::EulerAnglesf(), Nz::EulerAnglesf(float, float, float)>(),
			"pitch", &Nz::EulerAnglesf::pitch,
			"yaw", &Nz::EulerAnglesf::yaw,
			"roll", &Nz::EulerAnglesf::roll,
			sol::meta_function::to_string, &Nz::EulerAnglesf::ToString
		);
	}

	void MathScriptingLibrary::RegisterVector2(sol::state& state)
	{
		state.new_usertype<Nz::Vector2f>("Vec2",
			sol::call_constructor, sol::constructors<Nz::Vector2f(), Nz::Vector2f(float), Nz::Vector2f(float, float)>(),
			"x", &Nz::Vector2f::x,
			"y", &Nz::Vector2f::y,
			sol::meta_function::addition, Nz::Overload<const Nz::Vector2f&>(&Nz::Vector2f::operator+),
			sol::meta_function::division, sol::overload(Nz::Overload<float>(&Nz::Vector2f::operator/), Nz::Overload<const Nz::Vector2f&>(&Nz::Vector2f::operator/)),
			sol::meta_function::multiplication, sol::overload(Nz::Overload<float>(&Nz::Vector2f::operator*), Nz::Overload<const Nz::Vector2f&>(&Nz::Vector2f::operator*)),
			sol::meta_function::subtraction, Nz::Overload<const Nz::Vector2f&>(&Nz::Vector2f::operator-),
			sol::meta_function::to_string, &Nz::Vector2f::ToString,
			sol::meta_function::unary_minus, Nz::Overload<>(&Nz::Vector2f::operator-)
		);
	}

	void MathScriptingLibrary::RegisterVector3(sol::state& state)
	{
		state.new_usertype<Nz::Vector3f>("Vec3",
			sol::call_constructor, sol::constructors<Nz::Vector3f(), Nz::Vector3f(float), Nz::Vector3f(float, float, float)>(),
			"x", &Nz::Vector3f::x,
			"y", &Nz::Vector3f::y,
			"z", &Nz::Vector3f::z,
			sol::meta_function::addition, Nz::Overload<const Nz::Vector3f&>(&Nz::Vector3f::operator+),
			sol::meta_function::division, sol::overload(Nz::Overload<float>(&Nz::Vector3f::operator/), Nz::Overload<const Nz::Vector3f&>(&Nz::Vector3f::operator/)),
			sol::meta_function::multiplication, sol::overload(Nz::Overload<float>(&Nz::Vector3f::operator*), Nz::Overload<const Nz::Vector3f&>(&Nz::Vector3f::operator*)),
			sol::meta_function::subtraction, Nz::Overload<const Nz::Vector3f&>(&Nz::Vector3f::operator-),
			sol::meta_function::to_string, &Nz::Vector3f::ToString,
			sol::meta_function::unary_minus, Nz::Overload<>(&Nz::Vector3f::operator-)
		);
	}
}
