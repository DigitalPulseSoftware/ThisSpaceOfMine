// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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

		Nz::Quaternionf rotation;
		Nz::Vector3f translation;

		inline EnvironmentTransform& operator+=(const EnvironmentTransform& transform);
		inline EnvironmentTransform operator-() const;
	};
}

#include <CommonLib/EnvironmentTransform.inl>

#endif // TSOM_COMMONLIB_ENVIRONMENTTRANSFORM_HPP
