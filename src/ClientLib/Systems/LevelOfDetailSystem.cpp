// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/LevelOfDetailSystem.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/ClientConfigs.hpp>
#include <ClientLib/Components/ChunkLODComponent.hpp>
#include <CommonLib/ConfigFile.hpp>
#include <CommonLib/Components/ChunkContainerComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/ViewerInstance.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace tsom
{
	LevelOfDetailSystem::LevelOfDetailSystem(entt::registry& registry, ConfigFile& configFile) :
	m_registry(registry),
	m_configFile(configFile)
	{
	}

	void LevelOfDetailSystem::Update(Nz::Time /*elapsedTime*/)
	{
		struct CameraEntry
		{
			Nz::Vector3f position;
			float screenMultiple;
		};

		Nz::HybridVector<CameraEntry, 3> cameras;

		auto cameraView = m_registry.view<Nz::NodeComponent, Nz::CameraComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [cameraEntity, cameraNode, cameraComponent] : cameraView.each())
		{
			auto& viewerInstancePtr = cameraComponent.GetViewerInstance();
			const Nz::Matrix4f& projMatrix = viewerInstancePtr->GetProjectionMatrix();

			cameras.push_back({
				.position = cameraNode.GetGlobalPosition(),
				.screenMultiple = 0.5f * std::max(projMatrix(0, 0), projMatrix(1, 1))
			});
		}

		Nz::UInt8 minLevelOfDetail = m_configFile.GetIntegerValue<Nz::UInt8>(Config::Chunk_MinLevelOfDetail);

		auto chunkContainerView = m_registry.view<Nz::NodeComponent, ChunkContainerComponent, ChunkLODComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [chunkContainerEntity, chunkContainerNode, chunkContainer, chunkLodComponent] : chunkContainerView.each())
		{
			const Nz::Vector3f& containerPosition = chunkContainerNode.GetGlobalPosition();

			float screenRadius = 0.0f;
			for (const auto& cameraEntry : cameras)
				screenRadius = std::max(screenRadius, cameraEntry.screenMultiple * chunkLodComponent.radius / Nz::Vector3f::Distance(containerPosition, cameraEntry.position));

			screenRadius = 2.0f * screenRadius;

			Nz::UInt8 maxLod = Nz::SafeCaster(chunkLodComponent.chunkContainerLOD.size());
			Nz::UInt8 lodIndex = maxLod;
			for (; lodIndex > 0; --lodIndex)
			{
				if (screenRadius < chunkLodComponent.chunkContainerLOD[lodIndex - 1].screenSize)
					break;
			}

			lodIndex = std::max(lodIndex, std::min(minLevelOfDetail, maxLod));

			if (chunkLodComponent.activeLod != lodIndex)
			{
				chunkLodComponent.activeLod = lodIndex;
				for (auto& chunkEntitiesPtr : chunkContainer.chunkEntities)
				{
					if (!chunkEntitiesPtr)
						continue;

					ClientChunkEntities& clientChunkEntity = Nz::SafeCast<ClientChunkEntities&>(*chunkEntitiesPtr);

					if (lodIndex == 0)
					{
						clientChunkEntity.EnableCollisionGeneration(true);
						clientChunkEntity.UpdateChunkContainer(chunkContainer.chunkContainer);
					}
					else
					{
						clientChunkEntity.EnableCollisionGeneration(false);
						clientChunkEntity.UpdateChunkContainer(chunkLodComponent.chunkContainerLOD[lodIndex - 1].chunkContainer);
					}
				}
			}

			// Update chunk container up to active LOD level only (remember that lodIndex != lod entry index)
			for (Nz::UInt8 i = 0; i < lodIndex; ++i)
			{
				if (chunkLodComponent.chunkContainerLOD[i].chunkContainer)
					chunkLodComponent.chunkContainerLOD[i].chunkContainer->Update();
			}
		}
	}
}
