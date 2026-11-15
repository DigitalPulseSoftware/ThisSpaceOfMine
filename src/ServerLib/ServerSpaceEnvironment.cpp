// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerSpaceEnvironment.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Systems/ChunkEntitiesSystem.hpp>
#include <CommonLib/Utility/SpaceTransformations.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <PerlinNoise.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ServerSpaceEnvironment::ServerSpaceEnvironment(ServerInstance& serverInstance, const Nz::Vector3i32& coordinates) :
	ServerEnvironment(serverInstance, ServerEnvironmentType::Space, true),
	m_coordinates(coordinates)
	{
		m_world->GetRegistry().ctx().emplace<ServerSpaceEnvironment*>(this);
		m_world->AddSystem<ChunkEntitiesSystem>();
		m_world->AddSystem<EnvironmentSwitchSystem>();

		std::shared_ptr<const EntityClass> asteroidEntityClass = serverInstance.GetEntityRegistry().FindClass("asteroid");
		if (!asteroidEntityClass)
			return;

		constexpr std::size_t SampleSize = 16;

		siv::PerlinNoise asteroidProbabilityNoise;
		constexpr double NoiseScale = 0.005;

		for (std::size_t z = 1; z < SampleSize - 1; ++z)
		{
			for (std::size_t y = 1; y < SampleSize - 1; ++y)
			{
				for (std::size_t x = 1; x < SampleSize - 1; ++x)
				{
					Nz::Vector3f spawnPosition(x / float(SampleSize), y / float(SampleSize), z / float(SampleSize));
					spawnPosition *= Constants::SpaceCellSize;
					spawnPosition -= Nz::Vector3f(Constants::SpaceCellSize * 0.5f);

					Nz::Vector3d samplePos = SpaceTransformations::ToGlobalCoordinates(m_coordinates, spawnPosition) * NoiseScale;

					double value = asteroidProbabilityNoise.normalizedOctave3D_01(samplePos.x, samplePos.y, samplePos.z, 4, 0.1);
					if (value >= 0.8)
					{
						spdlog::info("created asteroid at {};{};{}", spawnPosition.x, spawnPosition.y, spawnPosition.z);

						entt::handle entity = CreateEntity();
						entity.emplace<Nz::NodeComponent>(spawnPosition, Nz::Quaternionf::Identity());
						entity.emplace<NetworkedComponent>();
						entity.emplace<ClassInstanceComponent>(asteroidEntityClass);

						asteroidEntityClass->InitAndActivateEntity(entity);
					}
				}
			}
		}
	}

	Nz::Boxf ServerSpaceEnvironment::ComputeBoundingBox() const
	{
		constexpr float HalfCellSize = Constants::SpaceCellSize * 0.5f;
		return Nz::Boxf::FromExtents(Nz::Vector3f(-HalfCellSize), Nz::Vector3f(HalfCellSize));
	}

	entt::handle ServerSpaceEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	ChunkContainer* ServerSpaceEnvironment::GetChunkContainer()
	{
		return nullptr;
	}

	const ChunkContainer* ServerSpaceEnvironment::GetChunkContainer() const
	{
		return nullptr;
	}

	const GravityController* ServerSpaceEnvironment::GetGravityController() const
	{
		return nullptr;
	}

	void ServerSpaceEnvironment::OnSave()
	{
		/* nothing to do */
	}

	void ServerSpaceEnvironment::OnTick(Nz::Time elapsedTime)
	{
		constexpr float SpaceCellSize_2 = Constants::SpaceCellSize * 0.5f;

		ServerEnvironment::OnTick(elapsedTime);

		entt::registry& registry = m_world->GetRegistry();
		auto switchView = registry.view<Nz::NodeComponent, ClassInstanceComponent, ServerEnvironmentSwitchComponent>();
		for (entt::entity entity : switchView)
		{
			auto& entityNode = switchView.get<Nz::NodeComponent>(entity);

			Nz::Vector3f entityPosition = entityNode.GetGlobalPosition();
			Nz::Vector3i32 cellOffset = Nz::Vector3i32(entityPosition / Nz::Vector3f(SpaceCellSize_2));
			if (cellOffset == Nz::Vector3i32::Zero())
				continue;

			ServerEnvironment* spaceEnvironment = m_serverInstance.GetOrCreateSpaceEnvironment(m_coordinates + cellOffset);

			auto& envSwitch = switchView.get<ServerEnvironmentSwitchComponent>(entity);
			envSwitch.Switch(entt::handle(registry, entity), this, spaceEnvironment, EnvironmentTransform(Nz::Vector3f(-cellOffset) * Nz::Vector3f(Constants::SpaceCellSize), Nz::Quaternionf::Identity()));
		}
	}

	ServerAtmosphere* ServerSpaceEnvironment::GetFallbackAtmosphereAtPosition(const Nz::Vector3f& /*position*/)
	{
		return nullptr;
	}
}
