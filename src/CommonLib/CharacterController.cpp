// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Planet.hpp>
#include <array>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace fixme
{
	Nz::Vector3f ProjectVec3(const Nz::Vector3f& vector, const Nz::Vector3f& normal)
	{
		float sqrLen = normal.GetSquaredLength();
		float dot = vector.DotProduct(normal);
		return normal * dot / sqrLen;
	}
}

namespace tsom
{
	CharacterController::CharacterController() :
	m_feetPosition(Nz::Vector3f::Down() * 1.8f * 0.5f),
	m_planet(nullptr)
	{
	}

	void CharacterController::PostSimulate(Nz::JoltCharacter& character)
	{
		if (m_planet)
		{
			Nz::Vector3f characterPosition = character.GetPosition() + character.GetRotation() * Nz::Vector3f::Down() * 0.9f;

			/*auto ExtractYawRotation = [&](const Nz::Quaternionf& quat)
			{
				Nz::RadianAnglef yaw(std::atan2(2.f * quat.y * quat.w - 2.f * quat.x * quat.z, 1.f - 2.f * quat.y * quat.y - 2.f * quat.z * quat.z));
				return Nz::Quaternionf(yaw, up);
			};*/

			Nz::Quaternionf previousRotation = character.GetRotation();
			Nz::Quaternionf rotation = previousRotation;

			Nz::Quaternionf rotationAroundUp = m_lastInputs.orientation;
			Nz::Vector3f forward = rotationAroundUp * Nz::Vector3f::Forward();

			if (Nz::Vector3f previousForward = rotation * Nz::Vector3f::Forward(); !previousForward.ApproxEqual(forward, 0.001f))
			{
				Nz::Quaternionf newRotation = Nz::Quaternionf::RotationBetween(previousForward, forward) * rotation;
				newRotation.Normalize();

				rotation = newRotation;
			}
			
			Nz::Vector3f up = m_planet->ComputeUpDirection(characterPosition);
			character.SetUp(up);

			if (Nz::Vector3f previousUp = rotation * Nz::Vector3f::Up(); !previousUp.ApproxEqual(up, 0.001f))
			{
				Nz::Quaternionf newRotation = Nz::Quaternionf::RotationBetween(previousUp, up) * rotation;
				newRotation.Normalize();

				rotation = newRotation;
			}

			if (rotation != previousRotation)
				character.SetRotation(rotation);
		}
	}

	void CharacterController::PreSimulate(Nz::JoltCharacter& character, float elapsedTime)
	{
		Nz::Vector3f velocity = character.GetLinearVelocity();
		Nz::Vector3f up = character.GetUp();

		if (m_planet)
		{
			// Apply gravity
			velocity -= m_planet->GetGravityFactor() * up * elapsedTime;

			if (m_lastInputs.jump)
			{
				if (character.IsOnGround())
					velocity += up * 10.f;
			}
		}

		float moveSpeed = 49.3f * 0.2f;

		if (m_lastInputs.sprint)
			moveSpeed *= 2.f;

		bool isMoving = false;

		Nz::Vector3f desiredVelocity = Nz::Vector3f::Zero();
		if (m_lastInputs.moveForward)
			desiredVelocity += Nz::Vector3f::Forward();

		if (m_lastInputs.moveBackward)
			desiredVelocity += Nz::Vector3f::Backward();

		if (m_lastInputs.moveLeft)
			desiredVelocity += Nz::Vector3f::Left();

		if (m_lastInputs.moveRight)
			desiredVelocity += Nz::Vector3f::Right();

		if (desiredVelocity != Nz::Vector3f::Zero())
		{
			character.SetFriction(0.f);
			desiredVelocity.Normalize();
		}
		else
			character.SetFriction(1.f);

		desiredVelocity = character.GetRotation() * desiredVelocity * moveSpeed;
		desiredVelocity += fixme::ProjectVec3(velocity, up);

		float desiredImpact = (character.IsOnGround()) ? 0.25f : 0.1f;

		character.SetLinearVelocity(Lerp(velocity, desiredVelocity, desiredImpact));
	}
}
