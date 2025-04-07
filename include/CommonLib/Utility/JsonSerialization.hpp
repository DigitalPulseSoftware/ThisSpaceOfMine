// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_JSONSERIALIZATION_HPP
#define TSOM_COMMONLIB_UTILITY_JSONSERIALIZATION_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Angle.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Rect.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Math/Vector4.hpp>
#include <nlohmann/json.hpp>

namespace Nz
{
	inline void from_json(const nlohmann::json& j, Nz::Color& color);
	template<typename T> void from_json(const nlohmann::json& j, Nz::DegreeAngle<T>& angle);
	template<typename T> void from_json(const nlohmann::json& j, Nz::Quaternion<T>& angle);
	template<typename T> void from_json(const nlohmann::json& j, Nz::Rect<T>& rect);
	template<typename T> void from_json(const nlohmann::json& j, Nz::Vector2<T>& vec);
	template<typename T> void from_json(const nlohmann::json& j, Nz::Vector3<T>& vec);
	template<typename T> void from_json(const nlohmann::json& j, Nz::Vector4<T>& vec);

	inline void to_json(nlohmann::json& j, const Nz::Color& color);
	template<typename T> void to_json(nlohmann::json& j, const Nz::DegreeAngle<T>& angle);
	template<typename T> void to_json(nlohmann::json& j, const Nz::Quaternion<T>& angle);
	template<typename T> void to_json(nlohmann::json& j, const Nz::Rect<T>& rect);
	template<typename T> void to_json(nlohmann::json& j, const Nz::Vector2<T>& vec);
	template<typename T> void to_json(nlohmann::json& j, const Nz::Vector3<T>& vec);
	template<typename T> void to_json(nlohmann::json& j, const Nz::Vector4<T>& vec);
}

#include <CommonLib/Utility/JsonSerialization.inl>

#endif // TSOM_COMMONLIB_UTILITY_JSONSERIALIZATION_HPP
