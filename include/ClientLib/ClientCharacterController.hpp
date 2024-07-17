// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTCHARACTERCONTROLLER_HPP
#define TSOM_CLIENTLIB_CLIENTCHARACTERCONTROLLER_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/CharacterController.hpp>

namespace Nz
{
	class DebugDrawer;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientCharacterController : public CharacterController
	{
		public:
			ClientCharacterController() = default;
			ClientCharacterController(const ClientCharacterController&) = delete;
			ClientCharacterController(ClientCharacterController&&) = delete;
			~ClientCharacterController() = default;

			void DebugDraw(Nz::PhysCharacter3D& character, Nz::DebugDrawer& debugDrawer);

			ClientCharacterController& operator=(const ClientCharacterController&) = delete;
			ClientCharacterController& operator=(ClientCharacterController&&) = delete;
	};
}

#include <ClientLib/ClientCharacterController.inl>

#endif // TSOM_CLIENTLIB_CLIENTCHARACTERCONTROLLER_HPP
