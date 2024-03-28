// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ServerLib/ServerShipEnvironment.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Ship.hpp>
#include <ServerLib/ServerInstance.hpp>

namespace tsom
{
	ServerShipEnvironment::ServerShipEnvironment(ServerInstance& serverInstance) :
	ServerEnvironment(serverInstance)
	{
		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_ship = std::make_unique<Ship>(blockLibrary, Nz::Vector3ui(32), 1.f);
		m_shipEntities = std::make_unique<ChunkEntities>(serverInstance.GetApplication(), m_world, *m_ship, blockLibrary);
	}

	ServerShipEnvironment::~ServerShipEnvironment()
	{
		m_shipEntities.reset();
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
