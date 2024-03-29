// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerShipEnvironment.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	ServerShipEnvironment::ServerShipEnvironment(ServerInstance& serverInstance) :
	ServerEnvironment(serverInstance)
	{
		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_ship = std::make_unique<Ship>(blockLibrary, Nz::Vector3ui(32), 1.f);
		m_shipEntities = std::make_unique<ChunkEntities>(serverInstance.GetApplication(), m_world, *m_ship, blockLibrary);

		auto& physicsSystem = m_world.GetSystem<Nz::Physics3DSystem>();
		auto& physWorld = physicsSystem.GetPhysWorld();
		physWorld.SetGravity(Nz::Vector3f::Down() * 9.81f);
	}

	ServerShipEnvironment::~ServerShipEnvironment()
	{
		m_shipEntities.reset();
	}

	entt::handle ServerShipEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	const GravityController* ServerShipEnvironment::GetGravityController() const
	{
		return m_ship.get();
	}

	Ship& ServerShipEnvironment::GetShip()
	{
		return *m_ship;
	}

	const Ship& ServerShipEnvironment::GetShip() const
	{
		return *m_ship;
	}

	void ServerShipEnvironment::OnLoad(const std::filesystem::path& loadPath)
	{
	}

	void ServerShipEnvironment::OnSave(const std::filesystem::path& savePath)
	{
	}

	void ServerShipEnvironment::OnTick(Nz::Time elapsedTime)
	{
		ServerEnvironment::OnTick(elapsedTime);

		m_shipEntities->Update();
	}
}
