// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_CHARACTERCONTROLLER_HPP
#define TSOM_CLIENT_CHARACTERCONTROLLER_HPP

#include <Nazara/JoltPhysics3D/JoltCharacter.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>

namespace Nz
{
	class DebugDrawer;
}

namespace tsom
{
	class Planet;

	class CharacterController : public Nz::JoltCharacterImpl
	{
		public:
			CharacterController();
			CharacterController(const CharacterController&) = delete;
			CharacterController(CharacterController&&) = delete;
			~CharacterController() = default;

			void DebugDraw(Nz::JoltCharacter& character, Nz::DebugDrawer& debugDrawer);

			void PostSimulate(Nz::JoltCharacter& character) override;
			void PreSimulate(Nz::JoltCharacter& character, float elapsedTime) override;

			inline void SetCurrentPlanet(const Planet* planet);

			CharacterController& operator=(const CharacterController&) = delete;
			CharacterController& operator=(CharacterController&&) = delete;

		private:
			struct RotationInProgress
			{
				Nz::Quaternionf fromRotation;
				Nz::Quaternionf toRotation;
				float progress = 0.f;
			};

			float m_totalElapsedTime = 0.f;
			std::optional<RotationInProgress> m_rotationInProgress;
			Nz::Boxf m_planetCube;
			Nz::Quaternionf m_cameraRotation;
			Nz::Quaternionf m_correctionRotation;
			Nz::Quaternionf m_referenceRotation;
			Nz::Vector3f m_feetPosition;
			Nz::Vector3f m_groundPos;
			const Planet* m_planet;
	};
}

#include <Client/CharacterController.inl>

#endif // TSOM_CLIENT_STATES_GAMESTATE_HPP
