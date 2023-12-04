// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_STATEDATA_HPP
#define TSOM_CLIENT_STATES_STATEDATA_HPP

#include <memory>

namespace Nz
{
	class ApplicationBase;
	class Canvas;
	class EnttWorld;
	class RenderTarget;
	class Window;
	class WindowSwapchain;
}

namespace tsom
{
	class ClientBlockLibrary;
	class ClientSessionHandler;
	class NetworkSession;

	struct StateData : std::enable_shared_from_this<StateData>
	{
		std::shared_ptr<Nz::RenderTarget> renderTarget;
		Nz::ApplicationBase* app;
		Nz::Canvas* canvas;
		Nz::EnttWorld* world;
		Nz::Window* window;
		ClientBlockLibrary* blockLibrary = nullptr;
		ClientSessionHandler* sessionHandler = nullptr;
		NetworkSession* networkSession = nullptr;
	};
}

#endif // TSOM_CLIENT_STATES_STATEDATA_HPP
