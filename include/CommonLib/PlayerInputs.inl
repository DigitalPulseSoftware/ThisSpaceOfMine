// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void PlayerInputs::Merge(const PlayerInputs& other)
	{
		index = other.index;
		if (data.index() != other.data.index())
			return;

		std::visit(Nz::Overloaded
		{
			[](std::monostate) {},
			[&](const Character& otherCharacter)
			{
				Character& character = std::get<Character>(data);
				character.crouch = otherCharacter.crouch;
				character.jump = character.jump || otherCharacter.jump;
				character.moveForward = otherCharacter.moveForward;
				character.moveBackward = otherCharacter.moveBackward;
				character.moveLeft = otherCharacter.moveLeft;
				character.moveRight = otherCharacter.moveRight;
				character.sprint = character.sprint || otherCharacter.sprint;
				character.pitch += otherCharacter.pitch;
				character.yaw += otherCharacter.yaw;
			},
			[&](const Ship& otherShip)
			{
				Ship& ship = std::get<Ship>(data);
				ship.moveForward = otherShip.moveForward;
				ship.moveBackward = otherShip.moveBackward;
				ship.moveLeft = otherShip.moveLeft;
				ship.moveRight = otherShip.moveRight;
				ship.moveUp = otherShip.moveUp;
				ship.moveDown = otherShip.moveDown;
				ship.rollLeft = otherShip.rollLeft;
				ship.rollRight = otherShip.rollRight;
				ship.stabilize = ship.stabilize || otherShip.stabilize;
				ship.pitch += otherShip.pitch;
				ship.yaw += otherShip.yaw;

			},
		}, other.data);
	}
}
