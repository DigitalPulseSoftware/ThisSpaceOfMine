// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERCHARACTERCONTROLLER_HPP
#define TSOM_SERVERLIB_SERVERCHARACTERCONTROLLER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/CharacterController.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerCharacterController : public CharacterController
	{
		public:
			using CharacterController::CharacterController;
	};
}

#include <ServerLib/ServerCharacterController.inl>

#endif // TSOM_SERVERLIB_SERVERCHARACTERCONTROLLER_HPP
