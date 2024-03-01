// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/GameAppComponent.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/EntitySystemAppComponent.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/PluginLoader.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Plugins/AssimpPlugin.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <Nazara/Platform/WindowingAppComponent.hpp>
#include <Nazara/Renderer/GpuSwitch.hpp>
#include <Nazara/Widgets/Widgets.hpp>
#include <Main/Main.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

NAZARA_REQUEST_DEDICATED_GPU()

int GameMain(int argc, char* argv[])
{
	// Engine setup
	Nz::Application<Nz::Graphics, Nz::Physics3D, Nz::Network, Nz::Widgets> app(argc, argv);

	Nz::PluginLoader pluginLoader;
	Nz::Plugin<Nz::AssimpPlugin> assimp = pluginLoader.Load<Nz::AssimpPlugin>();

	app.AddComponent<Nz::EntitySystemAppComponent>();
	app.AddComponent<Nz::FilesystemAppComponent>();
	app.AddComponent<Nz::TaskSchedulerAppComponent>();
	app.AddComponent<Nz::WindowingAppComponent>();

	try
	{
		app.AddComponent<Nz::WebServiceAppComponent>();
	}
	catch (const std::exception& e)
	{
		fmt::print(fg(fmt::color::red), "failed to enable web services (automatic update will be disabled): {0}!\n", e.what());
	}

	// Game setup
	app.AddComponent<tsom::GameAppComponent>();

	return app.Run();
}

TSOMMain(GameMain)
