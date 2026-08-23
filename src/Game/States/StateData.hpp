// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_STATEDATA_HPP
#define TSOM_GAME_STATES_STATEDATA_HPP

#include <entt/entt.hpp>
#include <memory>

namespace Nz
{
	class ApplicationBase;
	class Canvas;
	class EnttWorld;
	class RenderTarget;
	class TaskScheduler;
	class Window;
	class WindowSwapchain;
}

namespace tsom
{
	class ClientBlockLibrary;
	class ClientSessionHandler;
	class ConnectionState;
	class ConfigFile;
	class ImGuiRuntime;
	class NetworkSession;

	struct StateData : std::enable_shared_from_this<StateData>
	{
		std::shared_ptr<Nz::RenderTarget> renderTarget;
		entt::handle camera2D;
		Nz::ApplicationBase* app;
		Nz::Canvas* canvas;
		Nz::EnttWorld* world;
		Nz::Window* window;
		Nz::WindowSwapchain* swapchain;
		ClientBlockLibrary* blockLibrary = nullptr;
		ClientSessionHandler* sessionHandler = nullptr;
		ConfigFile* config;
		ConnectionState* connectionState = nullptr;
		NetworkSession* networkSession = nullptr;

#ifdef TSOM_DEV_TOOLS
		ImGuiRuntime* imgui = nullptr;
#endif
	};
}

#endif // TSOM_GAME_STATES_STATEDATA_HPP
