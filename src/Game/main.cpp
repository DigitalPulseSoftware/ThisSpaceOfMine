// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <ClientLib/Systems/MovementInterpolationSystem.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <Game/States/BackgroundState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/DebugInfoState.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/MenuState.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/EntitySystemAppComponent.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/PluginLoader.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderWindow.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <Nazara/Platform/MessageBox.hpp>
#include <Nazara/Platform/Platform.hpp>
#include <Nazara/Platform/WindowingAppComponent.hpp>
#include <Nazara/Renderer/GpuSwitch.hpp>
#include <Nazara/TextRenderer/TextRenderer.hpp>
#include <Nazara/Widgets/Widgets.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Core/Plugins/AssimpPlugin.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Systems/RenderSystem.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Main/Main.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <charconv>

NAZARA_REQUEST_DEDICATED_GPU()

int GameMain(int argc, char* argv[])
{
	Nz::Application<Nz::Graphics, Nz::Physics3D, Nz::Network, Nz::TextRenderer, Nz::Widgets> app(argc, argv);

	Nz::PluginLoader pluginLoader;
	Nz::Plugin<Nz::AssimpPlugin> assimp = pluginLoader.Load<Nz::AssimpPlugin>();

	Nz::TaskScheduler taskScheduler;

	auto& filesystem = app.AddComponent<Nz::FilesystemAppComponent>();

	std::filesystem::path assetPath = Nz::Utf8Path("assets");
	if (!std::filesystem::exists(assetPath))
	{
		fmt::print(fg(fmt::color::red), "assets are missing!\n");

		Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Missing assets folder", "The assets folder was not found, it should be located next to the executable.");
		errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);
		if (auto result = errorBox.Show(); !result)
			fmt::print(fg(fmt::color::red), "failed to open the error message box: {0}!\n", result.GetError());

		return EXIT_FAILURE;
	}

	filesystem.Mount("assets", assetPath);


	// Register a new SkyboxMaterial shader
	Nz::Graphics::Instance()->GetShaderModuleResolver()->RegisterModuleDirectory(Nz::Utf8Path("assets/shaders"), true);

	auto& commandLineParams = app.GetCommandLineParameters();
	unsigned int windowWidth = 1920;
	unsigned int windowHeight = 1080;

	std::string_view param;
	if (commandLineParams.GetParameter("width", &param))
	{
		if (auto err = std::from_chars(param.data(), param.data() + param.size(), windowWidth); err.ec != std::errc{})
			fmt::print(fg(fmt::color::red), "failed to parse width commandline parameter ({}) as number\n", param);
	}

	if (commandLineParams.GetParameter("height", &param))
	{
		if (auto err = std::from_chars(param.data(), param.data() + param.size(), windowHeight); err.ec != std::errc{})
			fmt::print(fg(fmt::color::red), "failed to parse height commandline parameter ({}) as number\n", param);
	}

	auto& windowComponent = app.AddComponent<Nz::WindowingAppComponent>();
	auto& window = windowComponent.CreateWindow(Nz::VideoMode(windowWidth, windowHeight), "This Space Of Mine");

	auto& ecsComponent = app.AddComponent<Nz::EntitySystemAppComponent>();

	try
	{
		app.AddComponent<Nz::WebServiceAppComponent>();
	}
	catch (const std::exception& e)
	{
		fmt::print(fg(fmt::color::red), "failed to enable web services (automatic update will be disabled): {0}!\n", e.what());
	}

	auto& world = ecsComponent.AddWorld<Nz::EnttWorld>();

	auto& renderSystem = world.AddSystem<Nz::RenderSystem>();

	Nz::SwapchainParameters swapchainParams;
	if (commandLineParams.HasFlag("no-vsync"))
		swapchainParams.presentMode = { Nz::PresentMode::Mailbox, Nz::PresentMode::Immediate };
	else
		swapchainParams.presentMode = { Nz::PresentMode::RelaxedVerticalSync, Nz::PresentMode::VerticalSync };

	auto& windowSwapchain = renderSystem.CreateSwapchain(window, swapchainParams);

	auto renderTarget = std::make_shared<Nz::RenderWindow>(windowSwapchain);

	tsom::PlanetGravitySystem planetGravity(world.GetRegistry());

	auto& physicsSystem = world.AddSystem<Nz::Physics3DSystem>();
	physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());
	physicsSystem.GetPhysWorld().SetStepSize(tsom::Constants::TickDuration);
	physicsSystem.GetPhysWorld().RegisterStepListener(&planetGravity);

	world.AddSystem<tsom::MovementInterpolationSystem>(tsom::Constants::TickDuration);
	world.AddSystem<tsom::AnimationSystem>();

	entt::handle camera2D = world.CreateEntity();
	{
		camera2D.emplace<Nz::NodeComponent>();
		//camera2D.emplace<Nz::DisabledComponent>();

		auto passList = filesystem.Load<Nz::PipelinePassList>("assets/2d.passlist");

		auto& cameraComponent = camera2D.emplace<Nz::CameraComponent>(renderTarget, std::move(passList), Nz::ProjectionType::Orthographic);
		cameraComponent.UpdateClearColor(Nz::Color(0.f, 0.f, 0.f, 0.f));
		cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask2D);
		cameraComponent.UpdateRenderOrder(1);
	}

	Nz::Canvas canvas(world.GetRegistry(), window.GetEventHandler(), window.GetCursorController().CreateHandle(), tsom::Constants::RenderMaskUI);
	canvas.Resize(Nz::Vector2f(window.GetSize()));
	window.GetEventHandler().OnResized.Connect([&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::SizeEvent& sizeEvent)
	{
		canvas.Resize(Nz::Vector2f(sizeEvent.width, sizeEvent.height));
	});

	tsom::ClientBlockLibrary blockLibrary(app, *Nz::Graphics::Instance()->GetRenderDevice());
	blockLibrary.BuildTexture();

	std::shared_ptr<tsom::StateData> stateData = std::make_shared<tsom::StateData>();
	stateData->app = &app;
	stateData->blockLibrary = &blockLibrary;
	stateData->canvas = &canvas;
	stateData->renderTarget = std::move(renderTarget);
	stateData->taskScheduler = &taskScheduler;
	stateData->window = &window;
	stateData->world = &world;

	std::shared_ptr<tsom::ConnectionState> connectionState = std::make_shared<tsom::ConnectionState>(stateData);

	Nz::StateMachine fsm;
	fsm.PushState(std::make_shared<tsom::DebugInfoState>(stateData));
	fsm.PushState(connectionState);
	fsm.PushState(std::make_shared<tsom::BackgroundState>(stateData));
	fsm.PushState(std::make_shared<tsom::MenuState>(stateData, connectionState));
	//Nz::StateMachine fsm(std::move(gameState));
	app.AddUpdaterFunc([&](Nz::Time time)
	{
		if (!fsm.Update(time))
			app.Quit();
	});

	return app.Run();
}

TSOMMain(GameMain)
