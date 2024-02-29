// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_GAMEAPPCOMPONENT_HPP
#define TSOM_GAME_GAMEAPPCOMPONENT_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <optional>

namespace Nz
{
	class EnttWorld;
	class RenderTarget;
	class WindowSwapchain;
	class Window;
}

namespace tsom
{
	class GameAppComponent : public Nz::ApplicationComponent
	{
		public:
			GameAppComponent(Nz::ApplicationBase& app);
			GameAppComponent(const GameAppComponent&) = delete;
			GameAppComponent(GameAppComponent&&) = delete;
			~GameAppComponent() = default;

			void Update(Nz::Time elapsedTime) override;

			GameAppComponent& operator=(const GameAppComponent&) = delete;
			GameAppComponent& operator=(GameAppComponent&&) = delete;

		private:
			bool CheckAssets();
			void SetupCamera(std::shared_ptr<const Nz::RenderTarget> renderTarget, Nz::EnttWorld& world);
			void SetupCanvas(Nz::EnttWorld& world, Nz::Window& window);
			std::shared_ptr<Nz::RenderTarget> SetupRenderTarget(Nz::EnttWorld& world, Nz::Window& window);
			Nz::Window& SetupWindow();
			Nz::EnttWorld& SetupWorld();

			Nz::StateMachine m_stateMachine;
			std::optional<Nz::Canvas> m_canvas;
			std::optional<ClientBlockLibrary> m_blockLibrary;
	};
}

#include <Game/GameAppComponent.inl>

#endif // TSOM_GAME_GAMEAPPCOMPONENT_HPP
