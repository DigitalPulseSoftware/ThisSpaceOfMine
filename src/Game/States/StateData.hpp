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
	class BaseWidget;
	class EnttWorld;
	class WindowSwapchain;
}

namespace tsom
{
	class NetworkSession;

	struct StateData : std::enable_shared_from_this<StateData>
	{
		Nz::ApplicationBase* app;
		Nz::BaseWidget* canvas;
		Nz::EnttWorld* world;
		Nz::WindowSwapchain* swapchain;
		NetworkSession* networkSession = nullptr;
	};
}

#endif // TSOM_CLIENT_STATES_STATEDATA_HPP
