// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/GravityController.hpp>
#include <Nazara/Physics3D/PhysWorld3D.hpp>
#include <Nazara/Physics3D/RigidBody3D.hpp>
#include <fmt/ostream.h>
#include <fmt/std.h>
#include <array>

#define DEBUG_ROTATION 0

namespace tsom
{
	CharacterController::CharacterController() :
	m_cameraRotation(Nz::EulerAnglesf::Zero()),
	m_referenceRotation(Nz::Quaternionf::Identity()),
	m_gravityController(nullptr),
	m_allowInputRotation(false),
	m_isFlying(false)
	{
	}

	void CharacterController::PostSimulate(Nz::PhysCharacter3D& character, float elapsedTime)
	{
		Nz::Vector3f charUp = character.GetUp();

		Nz::Quaternionf newRotation = m_referenceRotation;
		Nz::Vector3f newUp = charUp;

		// Update up vector if player is around a gravity well
		if (m_gravityForce.factor > 0.0f)
		{
			newUp = Nz::Vector3f::RotateTowards(charUp, -m_gravityForce.direction, Constants::GravityMaxRotationSpeed * elapsedTime);
			if (Nz::Vector3f previousUp = newRotation * Nz::Vector3f::Up(); !previousUp.ApproxEqual(newUp, 0.00001f))
			{
				Nz::Quaternionf upCorrected = Nz::Quaternionf::RotationBetween(previousUp, newUp) * newRotation;
				upCorrected.Normalize();

				newRotation = upCorrected;
			}
		}

		m_referenceRotation = newRotation;

		// Yaw (rotation around up vector)
		if (m_allowInputRotation && (!m_lastInputs.pitch.ApproxEqual(Nz::RadianAnglef::Zero()) || !m_lastInputs.yaw.ApproxEqual(Nz::RadianAnglef::Zero()))) //< Don't apply the same rotation twice
		{
#if DEBUG_ROTATION
			fmt::print("Applying pitch:{0},yaw:{1} from input {2} to {3}", m_lastInputs.pitch.ToDegrees(), m_lastInputs.yaw.ToDegrees(), m_lastInputs.index, fmt::streamed(m_cameraRotation));
#endif

			m_cameraRotation.pitch = Nz::Clamp(m_cameraRotation.pitch + m_lastInputs.pitch, -89.f, 89.f);
			m_cameraRotation.yaw += m_lastInputs.yaw;
			m_cameraRotation.Normalize();

#if DEBUG_ROTATION
			fmt::print(" => {0}\n", fmt::streamed(m_cameraRotation));
#endif

			m_allowInputRotation = false;
		}

		newRotation = m_referenceRotation * Nz::Quaternionf(m_cameraRotation.yaw, Nz::Vector3f::Up());
		newRotation.Normalize();

		if (!Nz::Quaternionf::ApproxEqual(newRotation, m_characterRotation, 0.00001f))
		{
			character.SetRotation(newRotation);
			character.SetUp(newUp);

			m_characterRotation = newRotation;
		}
	}

	void CharacterController::PreSimulate(Nz::PhysCharacter3D& character, float elapsedTime)
	{
		UpdatePosition(character);

		Nz::Vector3f velocity = character.GetLinearVelocity();
		Nz::Vector3f up = character.GetUp();

		m_gravityForce = (m_gravityController) ? m_gravityController->ComputeGravity(m_characterPosition) : GravityForce::Zero();

		Nz::Quaternionf movementRotation;
		if (m_gravityController)
		{
			if (!m_isFlying)
			{
				// Apply gravity
				Nz::Vector3f position = character.GetPosition();
				velocity += m_gravityForce.acceleration * m_gravityForce.direction * m_gravityForce.factor * elapsedTime;
			}

			movementRotation = character.GetRotation();
		}
		else
			movementRotation = character.GetRotation() * Nz::Quaternionf(Nz::EulerAnglesf(m_lastInputs.pitch, 0.f, 0.f));

		bool hasGravity = m_gravityForce.acceleration * m_gravityForce.factor > 0.2f;

		if (!m_isFlying && hasGravity)
		{
			if (m_lastInputs.jump)
			{
				if (character.IsOnGround())
					velocity += up * Constants::PlayerJumpPower;
			}
		}

		Nz::Vector3f desiredVelocity = Nz::Vector3f::Zero();
		// Handle up/down when flying before making movement rotation relative to pitch
		if (m_isFlying || !hasGravity)
		{
			if (m_lastInputs.jump)
				desiredVelocity += movementRotation * Nz::Vector3f::Up();

			if (m_lastInputs.crouch)
				desiredVelocity -= movementRotation * Nz::Vector3f::Up();

			movementRotation *= Nz::Quaternionf(m_cameraRotation.pitch, Nz::Vector3f::UnitX());
			movementRotation.Normalize();
		}

		if (m_lastInputs.moveForward)
			desiredVelocity += movementRotation * Nz::Vector3f::Forward();

		if (m_lastInputs.moveBackward)
			desiredVelocity += movementRotation * Nz::Vector3f::Backward();

		if (m_lastInputs.moveLeft)
			desiredVelocity += movementRotation * Nz::Vector3f::Left();

		if (m_lastInputs.moveRight)
			desiredVelocity += movementRotation * Nz::Vector3f::Right();

		if (desiredVelocity != Nz::Vector3f::Zero())
		{
			character.SetFriction(0.f);
			desiredVelocity.Normalize();
		}
		else
			character.SetFriction(1.f);

		if (m_isFlying)
			desiredVelocity *= Constants::PlayerFlySpeed * ((m_lastInputs.sprint) ? 2.f : 1.f);
		else if (m_lastInputs.sprint)
			desiredVelocity *= Constants::PlayerSprintSpeed;
		else
			desiredVelocity *= Constants::PlayerWalkSpeed;

		float desiredImpact;
		if (m_isFlying)
			desiredImpact = 0.2f;
		else if (hasGravity)
		{
			desiredVelocity += velocity.Project(m_gravityForce.direction);
			desiredImpact = (character.IsOnGround()) ? 0.25f : 0.1f;
		}
		else
		{
			desiredVelocity += velocity;
			desiredImpact = 0.01f;
		}

		character.SetLinearVelocity(Lerp(velocity, desiredVelocity, desiredImpact));
	}

	void CharacterController::RotateInstantaneously(const Nz::Quaternionf& rotation)
	{
		m_referenceRotation = rotation * m_referenceRotation;
		m_referenceRotation.Normalize();
	}

	void CharacterController::UpdatePosition(Nz::PhysCharacter3D& character)
	{
		std::tie(m_characterPosition, m_characterRotation) = character.GetPositionAndRotation();
	}
}
