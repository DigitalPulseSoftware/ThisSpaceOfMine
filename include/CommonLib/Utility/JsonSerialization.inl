// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace Nz
{
	inline void from_json(const nlohmann::json& j, Color& color)
	{
		j.at("r").get_to(color.r);
		j.at("g").get_to(color.g);
		j.at("b").get_to(color.b);
	}

	template<typename T>
	void from_json(const nlohmann::json& j, DegreeAngle<T>& angle)
	{
		angle = DegreeAngle<T>(T(j));
	}

	template<typename T>
	void from_json(const nlohmann::json& j, Quaternion<T>& quat)
	{
		j.at("x").get_to(quat.x);
		j.at("y").get_to(quat.y);
		j.at("z").get_to(quat.z);
		j.at("w").get_to(quat.w);
	}

	template<typename T>
	void from_json(const nlohmann::json& j, Rect<T>& rect)
	{
		j.at("x").get_to(rect.x);
		j.at("y").get_to(rect.y);
		j.at("width").get_to(rect.width);
		j.at("height").get_to(rect.height);
	}

	template<typename T>
	void from_json(const nlohmann::json& j, Vector2<T>& vec)
	{
		j.at("x").get_to(vec.x);
		j.at("y").get_to(vec.y);
	}

	template<typename T>
	void from_json(const nlohmann::json& j, Vector3<T>& vec)
	{
		j.at("x").get_to(vec.x);
		j.at("y").get_to(vec.y);
		j.at("z").get_to(vec.z);
	}

	template<typename T>
	void from_json(const nlohmann::json& j, Vector4<T>& vec)
	{
		j.at("x").get_to(vec.x);
		j.at("y").get_to(vec.y);
		j.at("z").get_to(vec.z);
		j.at("w").get_to(vec.w);
	}

	inline void to_json(nlohmann::json& j, const Color& color)
	{
		j = nlohmann::json{ {"r", color.r}, {"g", color.g}, {"b", color.b} };
	}

	template<typename T>
	void to_json(nlohmann::json& j, const DegreeAngle<T>& angle)
	{
		j = angle.ToDegrees();
	}

	template<typename T>
	void to_json(nlohmann::json& j, const Quaternion<T>& quat)
	{
		j = nlohmann::json{ {"x", quat.x}, {"y", quat.y}, {"z", quat.z}, {"w", quat.w} };
	}

	template<typename T>
	void to_json(nlohmann::json& j, const Rect<T>& rect)
	{
		j = nlohmann::json{ {"x", rect.x}, {"y", rect.y}, {"width", rect.width}, {"height", rect.height} };
	}

	template<typename T>
	void to_json(nlohmann::json& j, const Vector2<T>& vec)
	{
		j = nlohmann::json{ {"x", vec.x}, {"y", vec.y} };
	}

	template<typename T>
	void to_json(nlohmann::json& j, const Vector3<T>& vec)
	{
		j = nlohmann::json{ {"x", vec.x}, {"y", vec.y}, {"z", vec.z} };
	}

	template<typename T>
	void to_json(nlohmann::json& j, const Vector4<T>& vec)
	{
		j = nlohmann::json{ {"x", vec.x}, {"y", vec.y}, {"z", vec.z}, {"w", vec.w} };
	}
}
