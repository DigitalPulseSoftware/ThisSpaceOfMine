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
	class ServerEnvironment;

	class TSOM_SERVERLIB_API ServerCharacterController : public CharacterController
	{
		public:
			ServerCharacterController(ServerEnvironment* environment);
			ServerCharacterController(const ServerCharacterController&) = delete;
			ServerCharacterController(ServerCharacterController&&) = delete;
			~ServerCharacterController() = default;

			void PreSimulate(Nz::PhysCharacter3D& character, float elapsedTime) override;

			void UpdateEnvironment(ServerEnvironment* environment);

			ServerCharacterController& operator=(const ServerCharacterController&) = delete;
			ServerCharacterController& operator=(ServerCharacterController&&) = delete;

		private:
			ServerEnvironment* m_environment;
	};
}

#include <ServerLib/ServerCharacterController.inl>

#endif // TSOM_SERVERLIB_SERVERCHARACTERCONTROLLER_HPP
