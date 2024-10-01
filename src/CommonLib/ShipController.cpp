// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ShipController.hpp>
#include <CommonLib/CharacterController.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	void ShipController::PostSimulate(CharacterController& /*characterOwner*/, float /*elapsedTime*/)
	{
	}

	void ShipController::PreSimulate(CharacterController& character, float elapsedTime)
	{
		PlayerInputs inputs = character.GetInputs();
		if (!std::holds_alternative<PlayerInputs::Ship>(inputs.data))
			return;

		auto& shipInputs = std::get<PlayerInputs::Ship>(inputs.data);

		auto& rigidBody = m_entity.get<Nz::RigidBody3DComponent>();

		Nz::Vector3f force = Nz::Vector3f::Zero();
		if (shipInputs.moveForward)
			force += m_rotation * Nz::Vector3f::Forward();

		if (shipInputs.moveBackward)
			force += m_rotation * Nz::Vector3f::Backward();

		if (shipInputs.moveLeft)
			force += m_rotation * Nz::Vector3f::Left();

		if (shipInputs.moveRight)
			force += m_rotation * Nz::Vector3f::Right();

		force *= rigidBody.GetMass() * 10.f;

		rigidBody.AddForce(force, Nz::CoordSys::Local);

		constexpr float rollForce = 3.f;

		Nz::Vector3f torque = Nz::Vector3f::Zero();
		if (shipInputs.rollLeft)
			torque.z += rollForce;

		if (shipInputs.rollRight)
			torque.z -= rollForce;

		torque.x += shipInputs.pitch.ToDegrees();
		torque.y += shipInputs.yaw.ToDegrees();

		torque *= rigidBody.GetMass();

		rigidBody.AddTorque(m_rotation * torque, Nz::CoordSys::Local);
		rigidBody.WakeUp();
	}
}
