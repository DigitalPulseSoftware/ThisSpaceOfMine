#include <Nazara/Core.hpp>
#include <Nazara/JoltPhysics3D/JoltPhysics3D.hpp>
#include <Nazara/Network/Network.hpp>
#include <CommonLib/ServerWorldAppComponent.hpp>
#include <CommonLib/Session/InitialSessionHandler.hpp>
#include <Main/Main.hpp>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::JoltPhysics3D, Nz::Network> app(argc, argv);
	app.AddComponent<Nz::SignalHandlerAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerWorldAppComponent>();

	auto& world = worldAppComponent.AddWorld();
	auto& sessionManager = world.AddSessionManager(29536);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(world));

	return app.Run();
}

TSOMMain(ServerMain)
