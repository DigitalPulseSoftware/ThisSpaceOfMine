// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_WIDGETSTATE_HPP
#define TSOM_CLIENT_WIDGETSTATE_HPP

#include <Nazara/Core/State.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Client/ClientCharacter.hpp>
#include <Client/Planet.hpp>
#include <Client/VoxelGrid.hpp>
#include <entt/entt.hpp>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
	class JoltConvexHullCollider3D;
	class StaticMesh;
	class WindowEventHandler;
	class WindowSwapchain;
}

namespace tsom
{
	class GameState : public Nz::State
	{
		public:
			GameState(Nz::ApplicationBase& app, Nz::EnttWorld& world, Nz::WindowSwapchain& swapchain, Nz::WindowEventHandler& eventHandler);
			~GameState();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void RebuildPlanet();

			std::optional<ClientCharacter> m_character;
			std::unique_ptr<Planet> m_planet;
			entt::handle m_cameraEntity;
			entt::handle m_planetEntity;
			entt::handle m_skyboxEntity;
			Nz::ApplicationBase& m_app;
			Nz::EnttWorld& m_world;
			Nz::EulerAnglesf m_cameraRotation;
			Nz::WindowEventHandler& m_eventHandler;
	};
}

#include <Client/States/GameState.inl>

#endif // TSOM_CLIENT_STATES_GAMESTATE_HPP
