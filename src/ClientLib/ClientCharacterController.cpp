// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientCharacterController.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>

namespace tsom
{
	void ClientCharacterController::DebugDraw(Nz::JoltCharacter& character, Nz::DebugDrawer& debugDrawer)
	{
		Nz::Vector3f characterPosition = character.GetPosition() + character.GetRotation() * Nz::Vector3f::Down() * 0.9f;
		
		debugDrawer.DrawLine(characterPosition, characterPosition + character.GetUp(), Nz::Color::Green());
	}
}
