// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_CHARACTERCONTROLLER_HPP
#define TSOM_COMMONLIB_CHARACTERCONTROLLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <Nazara/JoltPhysics3D/JoltCharacter.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class Planet;

	class TSOM_COMMONLIB_API CharacterController : public Nz::JoltCharacterImpl
	{
		public:
			CharacterController();
			CharacterController(const CharacterController&) = delete;
			CharacterController(CharacterController&&) = delete;
			~CharacterController() = default;

			void PostSimulate(Nz::JoltCharacter& character) override;
			void PreSimulate(Nz::JoltCharacter& character, float elapsedTime) override;

			inline void SetCurrentPlanet(const Planet* planet);

			void SetInputs(const PlayerInputs& inputs);

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
			PlayerInputs m_lastInputs;
			const Planet* m_planet;
	};
}

#include <CommonLib/CharacterController.inl>

#endif // TSOM_COMMONLIB_STATES_GAMESTATE_HPP
