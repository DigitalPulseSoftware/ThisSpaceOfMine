// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Client/CharacterController.hpp>
#include <Client/Planet.hpp>
#include <Nazara/Platform/Keyboard.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <array>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace fixme
{
	Nz::Vector3f planetSize(50.f);

	constexpr std::array faceDirections = {
		Nz::Vector3f::Up(),
		Nz::Vector3f::Down(),
		Nz::Vector3f::Left(),
		Nz::Vector3f::Right(),
		Nz::Vector3f::Forward(),
		Nz::Vector3f::Backward()
	};

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
	m_cameraRotation(Nz::Quaternionf::Identity()),
	m_correctionRotation(Nz::Quaternionf::Identity()),
	m_referenceRotation(Nz::Quaternionf::Identity()),
	m_feetPosition(Nz::Vector3f::Down() * 1.8f * 0.5f)
	{
	}

	void CharacterController::DebugDraw(Nz::JoltCharacter& character, Nz::DebugDrawer& debugDrawer)
	{
		Nz::Vector3f characterPosition = character.GetPosition() + character.GetRotation() * Nz::Vector3f::Down() * 0.9f;
		
		debugDrawer.DrawLine(characterPosition, characterPosition + character.GetUp(), Nz::Color::Green());
	}

	void CharacterController::PostSimulate(Nz::JoltCharacter& character)
	{
		if (m_planet)
		{
			Nz::Vector3f center = Nz::Vector3f::Zero();
			Nz::Vector3f characterPosition = character.GetPosition() + character.GetRotation() * Nz::Vector3f::Down() * 0.9f;

			float distToCenter = std::max({
				std::abs(characterPosition.x - center.x), std::abs(center.x - characterPosition.x),
				std::abs(characterPosition.y - center.y), std::abs(center.y - characterPosition.y),
				std::abs(characterPosition.z - center.z), std::abs(center.z - characterPosition.z),
			});

			float innerReductionSize = std::max(distToCenter - std::max(m_planet->GetCornerRadius(), 1.f), 0.f);
			Nz::Boxf innerBox(center - Nz::Vector3f(innerReductionSize, innerReductionSize, innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

			Nz::Vector3f innerPos;
			innerPos.x = std::clamp(characterPosition.x, innerBox.GetMinimum().x, innerBox.GetMaximum().x);
			innerPos.y = std::clamp(characterPosition.y, innerBox.GetMinimum().y, innerBox.GetMaximum().y);
			innerPos.z = std::clamp(characterPosition.z, innerBox.GetMinimum().z, innerBox.GetMaximum().z);

			Nz::Vector3f up = Nz::Vector3f::Normalize(characterPosition - innerPos);
			character.SetUp(up);

			if (Nz::Vector3f previousUp = character.GetRotation() * Nz::Vector3f::Up(); previousUp != up)
			{
				fmt::print("previous up: {}.{}.{}\n", previousUp.x, previousUp.y, previousUp.z);
				fmt::print("up: {}.{}.{}\n", up.x, up.y, up.z);
				fmt::print("--\n");

				Nz::Quaternionf currentRotation = character.GetRotation();
				Nz::Quaternionf referenceRotation = (m_rotationInProgress) ? m_rotationInProgress->toRotation : currentRotation;
				Nz::Quaternionf newRotation = Nz::Quaternionf::RotationBetween(previousUp, up) * referenceRotation;
				newRotation.Normalize();

				character.SetRotation(newRotation);
				/*m_rotationInProgress = {
					currentRotation,
					newRotation,
					0.f
				};*/
			}
		}

		/*if (m_rotationInProgress)
		{
			constexpr float rotTime = 0.2f;

			m_rotationInProgress->progress = std::min(m_rotationInProgress->progress + m_totalElapsedTime / rotTime, 1.f);
			Nz::Quaternionf rotation;
			if (m_rotationInProgress->progress >= 1.f)
			{
				rotation = m_rotationInProgress->toRotation;
				m_rotationInProgress.reset();
			}
			else
			{
				rotation = Nz::Quaternionf::Slerp(m_rotationInProgress->fromRotation, m_rotationInProgress->toRotation, m_rotationInProgress->progress);
				rotation.Normalize();
			}

			character.SetRotation(rotation);
		}*/

		m_totalElapsedTime = 0.f;

		/*Nz::Boxf centerCube(fixme::planetSize);

		Nz::Vector3f centerPos;
		centerPos.x = std::clamp(characterPosition.x, centerCube.x, centerCube.x + centerCube.width);
		centerPos.y = std::clamp(characterPosition.y, centerCube.y, centerCube.y + centerCube.height);
		centerPos.z = std::clamp(characterPosition.z, centerCube.z, centerCube.z + centerCube.depth);

		float distToCenter = std::max(characterPosition.Distance(centerPos), 1.f);

		Nz::Vector3f rotCube(distToCenter, distToCenter, distToCenter);

		m_planetCube = Nz::Boxf(rotCube * -0.5f, rotCube);

		if (m_nextDown != m_currentDown)
		{
			m_correctionRotation = Nz::Quaternionf::RotationBetween(-fixme::faceDirections[m_currentDown], -fixme::faceDirections[m_nextDown]) * m_correctionRotation;
			m_correctionRotation.Normalize();
			m_currentDown = m_nextDown;
		}

		m_groundPos.x = std::clamp(characterPosition.x, m_planetCube.x, m_planetCube.x + m_planetCube.width);
		m_groundPos.y = std::clamp(characterPosition.y, m_planetCube.y, m_planetCube.y + m_planetCube.height);
		m_groundPos.z = std::clamp(characterPosition.z, m_planetCube.z, m_planetCube.z + m_planetCube.depth);

		if (characterPosition != m_groundPos)
		{
			Nz::Vector3f normal = (characterPosition - m_groundPos).Normalize();

			Nz::Quaternionf rotation = m_correctionRotation * Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), m_correctionRotation.GetConjugate() * normal);
			rotation.Normalize();
			character.SetRotation(rotation);
			character.SetUp(normal);

			m_referenceRotation = rotation;
		}*/
	}

	void CharacterController::PreSimulate(Nz::JoltCharacter& character, float elapsedTime)
	{
		m_totalElapsedTime += elapsedTime;

		// Handle gravity switch
		/*float maxDotProduct = -std::numeric_limits<float>::infinity();
		m_nextDown = m_currentDown;
		for (std::size_t i = 0; i < fixme::faceDirections.size(); ++i)
		{
			float minDotProduct = Nz::DegreeAnglef(45).GetCos();

			float dotProduct = dirToCenter.DotProduct(fixme::faceDirections[i]);
			if (dotProduct >= minDotProduct && dotProduct >= maxDotProduct)
			{
				maxDotProduct = dotProduct;
				m_nextDown = i;
			}
		}*/

		Nz::Vector3f velocity = character.GetLinearVelocity();
		Nz::Vector3f up = character.GetUp();
		
		if (m_planet)
		{
			// Apply gravity
			velocity -= 9.81f * up * elapsedTime;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space))
			{
				if (character.IsOnGround())
					velocity += up * 5.f;
			}
		}

		float moveSpeed = 49.3f * 0.2f;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift))
			moveSpeed *= 5.f;

		bool isMoving = false;

		Nz::Vector3f desiredVelocity = Nz::Vector3f::Zero();
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Z))
			desiredVelocity += Nz::Vector3f::Forward();

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::S))
			desiredVelocity += Nz::Vector3f::Backward();

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Q))
			desiredVelocity += Nz::Vector3f::Left();

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::D))
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
