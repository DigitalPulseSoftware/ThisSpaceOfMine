// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHARACTERCONTROLLER_HPP
#define TSOM_COMMONLIB_CHARACTERCONTROLLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Physics3D/PhysCharacter3D.hpp>
#include <entt/entt.hpp>
#include <optional>

namespace tsom
{
	class Planet;

	class TSOM_COMMONLIB_API CharacterController : public Nz::PhysCharacter3DImpl
	{
		public:
			CharacterController();
			CharacterController(const CharacterController&) = delete;
			CharacterController(CharacterController&&) = delete;
			~CharacterController() = default;

			inline void EnableFlying(bool enable = true);

			inline const Nz::EulerAnglesf& GetCameraRotation() const;
			inline const Nz::Vector3f& GetCharacterPosition() const;
			inline const Nz::Quaternionf& GetCharacterRotation() const;
			inline const Nz::Quaternionf& GetReferenceRotation() const;

			inline bool IsFlying() const;

			void PostSimulate(Nz::PhysCharacter3D& character, float elapsedTime) override;
			void PreSimulate(Nz::PhysCharacter3D& character, float elapsedTime) override;

			inline void SetCurrentPlanet(const Planet* planet);

			void SetInputs(const PlayerInputs& inputs);

			CharacterController& operator=(const CharacterController&) = delete;
			CharacterController& operator=(CharacterController&&) = delete;

		private:
			Nz::EulerAnglesf m_cameraRotation;
			Nz::Quaternionf m_referenceRotation;
			Nz::Quaternionf m_characterRotation;
			Nz::Vector3f m_characterPosition;
			PlayerInputs m_lastInputs;
			const Planet* m_planet;
			bool m_allowInputRotation;
			bool m_isFlying;
	};
}

#include <CommonLib/CharacterController.inl>

#endif // TSOM_COMMONLIB_CHARACTERCONTROLLER_HPP
