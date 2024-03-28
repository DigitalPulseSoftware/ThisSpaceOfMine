// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void CharacterController::EnableFlying(bool enable)
	{
		m_isFlying = enable;
	}

	inline const Nz::EulerAnglesf& CharacterController::GetCameraRotation() const
	{
		return m_cameraRotation;
	}

	inline const Nz::Vector3f& CharacterController::GetCharacterPosition() const
	{
		return m_characterPosition;
	}

	inline const Nz::Quaternionf& CharacterController::GetCharacterRotation() const
	{
		return m_characterRotation;
	}

	inline const Nz::Quaternionf& CharacterController::GetReferenceRotation() const
	{
		return m_referenceRotation;
	}

	inline bool CharacterController::IsFlying() const
	{
		return m_isFlying;
	}

	inline void CharacterController::SetGravityController(const GravityController* gravityController)
	{
		m_gravityController = gravityController;
	}

	inline void CharacterController::SetInputs(const PlayerInputs& inputs)
	{
		m_lastInputs = inputs;
		m_allowInputRotation = true;
	}
}
