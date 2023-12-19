// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Planet.hpp>
#include <Nazara/JoltPhysics3D/JoltPhysWorld3D.hpp>
#include <Nazara/JoltPhysics3D/JoltRigidBody3D.hpp>
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

	void CharacterController::PostSimulate(Nz::JoltCharacter& character, float elapsedTime)
	{
		auto [charPos, charRot] = character.GetPositionAndRotation();
		Nz::Vector3f characterPosition = charPos + charRot * Nz::Vector3f::Down() * 0.9f;
		Nz::Vector3f charUp = character.GetUp();

		Nz::Quaternionf newRotation = charRot;
		Nz::Vector3f newUp = charUp;

		// Update up vector if player is around a gravity well
		if (m_planet)
		{
			Nz::Vector3f targetUp = m_planet->ComputeUpDirection(characterPosition);
			newUp = Nz::Vector3f::RotateTowards(charUp, targetUp, Nz::DegreeAnglef(90.f) * elapsedTime);

			if (Nz::Vector3f previousUp = newRotation * Nz::Vector3f::Up(); !previousUp.ApproxEqual(newUp, 0.00001f))
			{
				Nz::Quaternionf upCorrected = Nz::Quaternionf::RotationBetween(previousUp, newUp) * newRotation;
				upCorrected.Normalize();

				newRotation = upCorrected;
			}
		}

		// Yaw (rotation around up vector)
		if (!m_lastInputs.yaw.ApproxEqual(Nz::RadianAnglef::Zero()))
		{
			newRotation = newRotation * Nz::Quaternionf(m_lastInputs.yaw, Nz::Vector3f::Up());
			newRotation.Normalize();
		}

		if (!Nz::Quaternionf::ApproxEqual(newRotation, charRot, 0.00001f))
		{
			character.SetRotation(newRotation);
			character.SetUp(newUp);
		}
	}

	void CharacterController::PreSimulate(Nz::JoltCharacter& character, float elapsedTime)
	{
		Nz::Vector3f velocity = character.GetLinearVelocity();
		Nz::Vector3f up = character.GetUp();

		Nz::Quaternionf movementRotation;
		if (m_planet)
		{
			// Apply gravity
			Nz::Vector3f position = character.GetPosition();
			velocity -= m_planet->GetGravityFactor(position) * m_planet->ComputeUpDirection(character.GetPosition()) * elapsedTime;

			if (m_lastInputs.jump)
			{
				if (character.IsOnGround())
					velocity += up * 10.f;
			}

			// Restrict movement around up
			auto ExtractYawRotation = [&](const Nz::Quaternionf& quat)
			{
				Nz::RadianAnglef yaw(std::atan2(2.f * quat.y * quat.w - 2.f * quat.x * quat.z, 1.f - 2.f * quat.y * quat.y - 2.f * quat.z * quat.z));
				return Nz::Quaternionf(yaw, up);
			};

			movementRotation = character.GetRotation();
		}
		else
			movementRotation = character.GetRotation() * Nz::Quaternionf(Nz::EulerAnglesf(m_lastInputs.pitch, 0.f, 0.f));

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

		desiredVelocity = movementRotation * desiredVelocity * moveSpeed;
		desiredVelocity += fixme::ProjectVec3(velocity, up);

		float desiredImpact = (character.IsOnGround()) ? 0.25f : 0.1f;

		character.SetLinearVelocity(Lerp(velocity, desiredVelocity, desiredImpact));
	}
}
