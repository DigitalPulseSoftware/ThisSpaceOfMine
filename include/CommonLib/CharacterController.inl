// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline void CharacterController::SetCurrentPlanet(const Planet* planet)
	{
		m_planet = planet;
	}

	inline void CharacterController::SetInputs(const PlayerInputs& inputs)
	{
		m_lastInputs = inputs;
		m_allowInputRotation = true;
	}
}
