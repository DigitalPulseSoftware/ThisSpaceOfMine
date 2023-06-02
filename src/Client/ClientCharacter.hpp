// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_CLIENTCHARACTER_HPP
#define TSOM_CLIENT_CLIENTCHARACTER_HPP

#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>

namespace Nz
{
	class DebugDrawer;
	class EnttWorld;
	class JoltCollider3D;
}

namespace tsom
{
	class CharacterController;
	class Planet;

	class ClientCharacter
	{
		public:
			ClientCharacter(Nz::EnttWorld& world, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);
			ClientCharacter(const ClientCharacter&) = delete;
			ClientCharacter(ClientCharacter&&) = delete;
			~ClientCharacter() = default;

			void DebugDraw(Nz::DebugDrawer& debugDrawer);

			inline entt::handle GetEntity() const;

			void SetCurrentPlanet(const Planet* planet);

			ClientCharacter& operator=(const ClientCharacter&) = delete;
			ClientCharacter& operator=(ClientCharacter&&) = delete;

			static std::shared_ptr<Nz::JoltCollider3D> BuildCollider();

		private:
			std::shared_ptr<CharacterController> m_controller;
			entt::handle m_entity;
	};
}

#include <Client/ClientCharacter.inl>

#endif // TSOM_CLIENT_STATES_GAMESTATE_HPP
