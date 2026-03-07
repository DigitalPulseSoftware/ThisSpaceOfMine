// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENVIRONMENTTRANSFORM_HPP
#define TSOM_COMMONLIB_ENVIRONMENTTRANSFORM_HPP

#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	struct EnvironmentTransform
	{
		EnvironmentTransform() = default;
		inline EnvironmentTransform(const Nz::Vector3f& translation, const Nz::Quaternionf& rotation);

		inline bool ApproxEqual(const EnvironmentTransform& other, float translationEpsilon = 0.01f, float rotationEpsilon = 0.0001f) const;

		inline Nz::Quaternionf Rotate(const Nz::Quaternionf& localRotation) const;
		inline Nz::Vector3f Rotate(const Nz::Vector3f& direction) const;
		inline Nz::Vector3f Translate(const Nz::Vector3f& localPosition) const;

		inline EnvironmentTransform operator+(const EnvironmentTransform& transform) const;
		inline EnvironmentTransform& operator+=(const EnvironmentTransform& transform);
		inline EnvironmentTransform operator-(const EnvironmentTransform& transform) const;
		inline EnvironmentTransform& operator-=(const EnvironmentTransform& transform);
		inline EnvironmentTransform operator-() const;

		static inline EnvironmentTransform Identity();

		Nz::Quaternionf rotation;
		Nz::Vector3f translation;
	};
}

#include <CommonLib/EnvironmentTransform.inl>

#endif // TSOM_COMMONLIB_ENVIRONMENTTRANSFORM_HPP
