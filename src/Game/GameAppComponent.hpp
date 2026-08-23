// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_GAMEAPPCOMPONENT_HPP
#define TSOM_GAME_GAMEAPPCOMPONENT_HPP

#include <ClientLib/ClientBlockLibrary.hpp>
#include <CommonLib/Systems/GravityPhysicsSystem.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <optional>

#ifdef TSOM_DEV_TOOLS
#include <ClientLib/ImGuiRuntime.hpp>
#endif

namespace Nz
{
	class EnttWorld;
	class RenderTarget;
	class WindowSwapchain;
	class Window;
	class WindowSwapchain;
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

			void Start();

			void Update(Nz::Time elapsedTime) override;

			GameAppComponent& operator=(const GameAppComponent&) = delete;
			GameAppComponent& operator=(GameAppComponent&&) = delete;

		private:
			bool CheckAssets();
			void SetupCamera(std::shared_ptr<const Nz::RenderTarget> renderTarget, Nz::EnttWorld& world);
			void SetupCanvas(Nz::EnttWorld& world, Nz::Window& window);
			Nz::WindowSwapchain& SetupSwapchain(Nz::EnttWorld& world, Nz::Window& window);
			Nz::Window& SetupWindow();
			Nz::EnttWorld& SetupWorld();

			NazaraSlot(Nz::WindowEventHandler, OnDestruction, m_onWindowDestruction);

			std::optional<Nz::Canvas> m_canvas;
			std::optional<ClientBlockLibrary> m_blockLibrary;
#ifdef TSOM_DEV_TOOLS
			std::optional<ImGuiRuntime> m_imguiRuntime;
#endif
			Nz::Signal<long long>::ConnectionGuard m_fpsLimitUpdateSlot;
			Nz::StateMachine m_stateMachine;

			Nz::HighPrecisionClock m_frameClock;
			Nz::Int16 m_fpsLimit;
	};
}

#include <Game/GameAppComponent.inl>

#endif // TSOM_GAME_GAMEAPPCOMPONENT_HPP
