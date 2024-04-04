// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline EnvironmentTransform::EnvironmentTransform(const Nz::Vector3f& Translation, const Nz::Quaternionf& Rotation) :
	rotation(Rotation),
	translation(Translation)
	{
	}

	inline EnvironmentTransform& EnvironmentTransform::operator+=(const EnvironmentTransform& transform)
	{
		translation += rotation * transform.translation;
		rotation *= transform.rotation;
		rotation.Normalize();

		return *this;
	}

	inline EnvironmentTransform EnvironmentTransform::operator-() const
	{
		EnvironmentTransform reverseTransform;
		reverseTransform.rotation = rotation.GetConjugate();
		reverseTransform.translation = -(reverseTransform.rotation * translation);

		return reverseTransform;
	}
}
