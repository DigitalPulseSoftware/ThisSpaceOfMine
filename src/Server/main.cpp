#include <Nazara/Core.hpp>
#include <Nazara/JoltPhysics3D/JoltPhysics3D.hpp>
#include <Nazara/Network/Network.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <Main/Main.hpp>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::JoltPhysics3D, Nz::Network> app(argc, argv);
	app.AddComponent<Nz::SignalHandlerAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerInstanceAppComponent>();

	auto& instance = worldAppComponent.AddInstance();
	auto& sessionManager = instance.AddSessionManager(29536);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	return app.Run();
}

TSOMMain(ServerMain)
