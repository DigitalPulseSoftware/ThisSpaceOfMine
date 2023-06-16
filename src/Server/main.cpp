#include <Nazara/Core.hpp>
#include <Nazara/Network/Network.hpp>
#include <CommonLib/PlayerSessionHandler.hpp>
#include <CommonLib/ServerWorldAppComponent.hpp>
#include <Main/Main.hpp>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Network> app(argc, argv);
	app.AddComponent<Nz::SignalHandlerAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerWorldAppComponent>();

	auto& world = worldAppComponent.AddWorld();
	auto& sessionManager = world.AddSessionManager(29536);
	sessionManager.SetDefaultHandler<tsom::PlayerSessionHandler>();

	return app.Run();
}

TSOMMain(ServerMain)
