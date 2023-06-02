#include <Nazara/Core.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Platform.hpp>
#include <Nazara/JoltPhysics3D.hpp>
#include <Nazara/Renderer.hpp>
#include <Nazara/Utility.hpp>
#include <Nazara/Widgets.hpp>
#include <Client/States/BackgroundState.hpp>
#include <Client/States/GameState.hpp>
#include <Client/States/MenuState.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
	Nz::Renderer::Config config;
	config.preferredAPI = Nz::RenderAPI::OpenGL;

	Nz::Application<Nz::Graphics, Nz::JoltPhysics3D, Nz::Widgets> app(argc, argv, config);

	auto& filesystem = app.AddComponent<Nz::AppFilesystemComponent>();
	filesystem.Mount("assets", Nz::Utf8Path("../Assets"));

	// Register a new SkyboxMaterial shader
	Nz::Graphics::Instance()->GetShaderModuleResolver()->RegisterModuleDirectory(Nz::Utf8Path("../Assets/Shaders"), true);

	auto& windowComponent = app.AddComponent<Nz::AppWindowingComponent>();
	auto& window = windowComponent.CreateWindow(Nz::VideoMode(1920, 1080), "This Space Of Mine");

	auto& ecsComponent = app.AddComponent<Nz::AppEntitySystemComponent>();

	auto& world = ecsComponent.AddWorld<Nz::EnttWorld>();

	auto& renderSystem = world.AddSystem<Nz::RenderSystem>();
	auto& windowSwapchain = renderSystem.CreateSwapchain(window);

	auto& physicsSystem = world.AddSystem<Nz::JoltPhysics3DSystem>();
	physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());

	std::cout << "Hello TSOM" << std::endl;

	entt::handle camera2D = world.CreateEntity();
	{
		camera2D.emplace<Nz::NodeComponent>();
		camera2D.emplace<Nz::DisabledComponent>();

		auto& cameraComponent = camera2D.emplace<Nz::CameraComponent>(&windowSwapchain, Nz::ProjectionType::Orthographic);
		cameraComponent.UpdateClearColor(Nz::Color(0.f, 0.f, 0.f, 0.f));
		cameraComponent.UpdateRenderMask(0xFFFF0000);
		cameraComponent.UpdateRenderOrder(1);
	}

	Nz::Canvas canvas(world.GetRegistry(), window.GetEventHandler(), window.GetCursorController().CreateHandle(), 0xFFFF0000);
	canvas.Resize(Nz::Vector2f(window.GetSize()));

	std::shared_ptr<Nz::State> gameState = std::make_shared<tsom::GameState>(app, world, windowSwapchain, window.GetEventHandler());

	Nz::Mouse::SetRelativeMouseMode(true);

	//Nz::StateMachine fsm(std::make_shared<tsom::BackgroundState>(app, &canvas, world, windowSwapchain));
	//fsm.PushState(std::make_shared<tsom::MenuState>(&canvas, world, std::move(gameState)));
	Nz::StateMachine fsm(std::move(gameState));
	app.AddUpdaterFunc([&](Nz::Time time)
	{
		fsm.Update(time);
	});

	return app.Run();
}
