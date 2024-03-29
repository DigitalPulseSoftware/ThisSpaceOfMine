// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerShipEnvironment.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Systems/ShipSystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	ServerShipEnvironment::ServerShipEnvironment(ServerInstance& serverInstance) :
	ServerEnvironment(serverInstance)
	{
		auto& app = serverInstance.GetApplication();
		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_shipEntity = CreateEntity();
		m_shipEntity.emplace<Nz::NodeComponent>();
		m_shipEntity.emplace<NetworkedComponent>();

		auto& shipComponent = m_shipEntity.emplace<ShipComponent>(1.f);
		shipComponent.Generate(blockLibrary);

		shipComponent.shipEntities = std::make_unique<ChunkEntities>(app, m_world, shipComponent, blockLibrary);
		shipComponent.shipEntities->SetParentEntity(m_shipEntity);

		m_world.AddSystem<ShipSystem>();
	}

	ServerShipEnvironment::~ServerShipEnvironment()
	{
		m_shipEntity.destroy();
	}

	entt::handle ServerShipEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	const GravityController* ServerShipEnvironment::GetGravityController() const
	{
		return &m_shipEntity.get<ShipComponent>();
	}

	Ship& ServerShipEnvironment::GetShip()
	{
		return m_shipEntity.get<ShipComponent>();
	}

	const Ship& ServerShipEnvironment::GetShip() const
	{
		return m_shipEntity.get<ShipComponent>();
	}

	void ServerShipEnvironment::OnLoad(const std::filesystem::path& loadPath)
	{
	}

	void ServerShipEnvironment::OnSave(const std::filesystem::path& savePath)
	{
	}
}
