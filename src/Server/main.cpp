// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/InternalConstants.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/Core.hpp>
#include <Nazara/Core/SignalHandlerAppComponent.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <Main/Main.hpp>
#include <fmt/color.h>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Physics3D, Nz::Network> app(argc, argv);
	app.AddComponent<Nz::SignalHandlerAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerInstanceAppComponent>();

	auto& instance = worldAppComponent.AddInstance();
	auto& sessionManager = instance.AddSessionManager(tsom::Constants::ServerPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	fmt::print(fg(fmt::color::lime_green), "server ready.\n");

	return app.Run();
}

TSOMMain(ServerMain)
