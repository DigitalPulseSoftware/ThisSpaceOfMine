// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void CharacterController::EnableFlying(bool enable)
	{
		m_isFlying = enable;
	}

	inline const Nz::EulerAnglesf& CharacterController::GetCameraAngles() const
	{
		return m_cameraAngles;
	}

	inline Nz::Quaternionf CharacterController::GetCameraRotation() const
	{
		// TODO: Use quaternion to rotate pitch
		Nz::Quaternionf camRot = m_characterRotation * Nz::EulerAnglesf(m_cameraAngles.pitch, 0.f, 0.f);
		camRot.Normalize();

		return camRot;
	}

	inline const Nz::Vector3f& CharacterController::GetCharacterPosition() const
	{
		return m_characterPosition;
	}

	inline const Nz::Quaternionf& CharacterController::GetCharacterRotation() const
	{
		return m_characterRotation;
	}

	inline const GravityController* CharacterController::GetGravityController() const
	{
		return m_gravityController;
	}

	inline const PlayerInputs& CharacterController::GetInputs() const
	{
		return m_lastInputs;
	}

	inline const Nz::Quaternionf& CharacterController::GetReferenceRotation() const
	{
		return m_referenceRotation;
	}

	inline const std::shared_ptr<ShipController>& CharacterController::GetShipController() const
	{
		return m_shipController;
	}

	inline bool CharacterController::IsFlying() const
	{
		return m_isFlying;
	}

	inline bool CharacterController::IsInWater() const
	{
		return m_isInWater;
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

	inline void tsom::CharacterController::SetInWater(bool isInWater)
	{
		m_isInWater = isInWater;
	}
}
