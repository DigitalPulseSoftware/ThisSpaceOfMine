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
#include <Game/States/GameState.hpp>
#include <Game/States/MenuState.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

int GameMain(int argc, char* argv[])
{
	Nz::Application<Nz::Graphics, Nz::JoltPhysics3D, Nz::Network, Nz::Widgets> app(argc, argv);

	tsom::NetworkReactor reactor(0, Nz::NetProtocol::Any, 0, 1);

	reactor.ConnectTo(Nz::IpAddress("[::1]:29536"));

	app.AddUpdaterFunc([&]
	{
		auto ConnectionHandler = [&]([[maybe_unused]] bool outgoingConnection, std::size_t peerIndex, const Nz::IpAddress& remoteAddress, [[maybe_unused]] Nz::UInt32 data)
		{
			fmt::print("Peer connected (outgoing: {}, peerIndex: {}, address: {}, data: {})\n", outgoingConnection, peerIndex, fmt::streamed(remoteAddress), data);

			/*tsom::Packets::Test testPacket;
			testPacket.str = "Mishaa est un gros nul";

			Nz::NetPacket packet;
			packet << Nz::UInt8(Nz::TypeListFind<tsom::PacketTypes, tsom::Packets::Test>);

			tsom::PacketSerializer serializer(packet, true);
			tsom::Packets::Serialize(serializer, testPacket);*/

			tsom::Packets::NetworkStrings testPacket;
			testPacket.startId = 0;
			testPacket.strings.emplace_back("Mishaa est un gros nul");

			Nz::NetPacket packet;
			packet << Nz::UInt8(Nz::TypeListFind<tsom::PacketTypes, tsom::Packets::NetworkStrings>);

			tsom::PacketSerializer serializer(packet, true);
			tsom::Packets::Serialize(serializer, testPacket);

			reactor.SendData(peerIndex, 0, Nz::ENetPacketFlag_Reliable, std::move(packet));
		};

		auto DisconnectionHandler = [&](std::size_t peerIndex, [[maybe_unused]] Nz::UInt32 data)
		{
			fmt::print("Peer disconnected (peerIndex: {}, data: {})\n", peerIndex, data);
		};

		auto PacketHandler = [&](std::size_t peerIndex, Nz::NetPacket&& packet)
		{
			fmt::print("Received packet\n", peerIndex);
		};

		reactor.Poll(ConnectionHandler, DisconnectionHandler, PacketHandler);
	});

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

	std::shared_ptr<Nz::State> gameState = std::make_shared<tsom::GameState>(app, world, windowSwapchain, window.GetEventHandler());

	//Nz::Mouse::SetRelativeMouseMode(true);

	Nz::StateMachine fsm(std::make_shared<tsom::BackgroundState>(app, &canvas, world, windowSwapchain));
	fsm.PushState(std::make_shared<tsom::MenuState>(&canvas, world, std::move(gameState)));
	//Nz::StateMachine fsm(std::move(gameState));
	app.AddUpdaterFunc([&](Nz::Time time)
	{
		fsm.Update(time);
	});

	return app.Run();
}

TSOMMain(GameMain)
