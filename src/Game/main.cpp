#include <Nazara/Core.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Platform.hpp>
#include <Nazara/JoltPhysics3D.hpp>
#include <Nazara/Network.hpp>
#include <Nazara/Renderer.hpp>
#include <Nazara/Utility.hpp>
#include <Nazara/Widgets.hpp>
#include <Main/Main.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Game/States/BackgroundState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/DebugInfoState.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/MenuState.hpp>
#include <Game/States/StateData.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

int GameMain(int argc, char* argv[])
{
	Nz::Application<Nz::Graphics, Nz::JoltPhysics3D, Nz::Network, Nz::Widgets> app(argc, argv);

	auto& filesystem = app.AddComponent<Nz::AppFilesystemComponent>();
	filesystem.Mount("assets", Nz::Utf8Path("../Assets"));

	// Register a new SkyboxMaterial shader
	Nz::Graphics::Instance()->GetShaderModuleResolver()->RegisterModuleDirectory(Nz::Utf8Path("../Assets/Shaders"), true);

	auto& windowComponent = app.AddComponent<Nz::AppWindowingComponent>();
	auto& window = windowComponent.CreateWindow(Nz::VideoMode(1920, 1080), "This Space Of Mine");

	auto& ecsComponent = app.AddComponent<Nz::AppEntitySystemComponent>();

	auto& world = ecsComponent.AddWorld<Nz::EnttWorld>();

	auto& renderSystem = world.AddSystem<Nz::RenderSystem>();

	Nz::SwapchainParameters swapchainParams;
	if (app.GetCommandLineParameters().HasFlag("no-vsync"))
		swapchainParams.presentMode = { Nz::PresentMode::Mailbox, Nz::PresentMode::Immediate };
	else
		swapchainParams.presentMode = { Nz::PresentMode::RelaxedVerticalSync, Nz::PresentMode::VerticalSync };

	auto& windowSwapchain = renderSystem.CreateSwapchain(window, swapchainParams);

	auto& physicsSystem = world.AddSystem<Nz::JoltPhysics3DSystem>();
	physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());

	entt::handle camera2D = world.CreateEntity();
	{
		camera2D.emplace<Nz::NodeComponent>();
		//camera2D.emplace<Nz::DisabledComponent>();

		auto& cameraComponent = camera2D.emplace<Nz::CameraComponent>(&windowSwapchain, Nz::ProjectionType::Orthographic);
		cameraComponent.UpdateClearColor(Nz::Color(0.f, 0.f, 0.f, 0.f));
		cameraComponent.UpdateRenderMask(0xFFFF0000);
		cameraComponent.UpdateRenderOrder(1);
	}

	Nz::Canvas canvas(world.GetRegistry(), window.GetEventHandler(), window.GetCursorController().CreateHandle(), 0xFFFF0000);
	canvas.Resize(Nz::Vector2f(window.GetSize()));

	std::shared_ptr<tsom::StateData> stateData = std::make_shared<tsom::StateData>();
	stateData->app = &app;
	stateData->canvas = &canvas;
	stateData->swapchain = &windowSwapchain;
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
		fsm.Update(time);
	});

	return app.Run();
}

TSOMMain(GameMain)
