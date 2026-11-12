// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
#define TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP

#include <ClientLib/ClientBlockLibrary.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <Nazara/Core/Color.hpp>
#include <tsl/hopscotch_map.h>
#include <mutex>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
	class GpuAsyncCommands;
	class GraphicalMesh;
	class Mesh;
	class MaterialInstance;
	class TaskScheduler;
	class VertexDeclaration;
}

namespace tsom
{
	struct VertexStruct
	{
		Nz::Vector3f position;
		Nz::Vector3f normal;
		Nz::UInt32 blockIndex;
	};

	class ConfigFile;

	class TSOM_CLIENTLIB_API ClientChunkEntities final : public ChunkEntities
	{
		public:
			ClientChunkEntities(Nz::ApplicationBase& app, ConfigFile& config, Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, const ClientBlockLibrary& blockLibrary, std::size_t layerIndex, bool isStatic);
			ClientChunkEntities(const ClientChunkEntities&) = delete;
			ClientChunkEntities(ClientChunkEntities&&) = delete;
			~ClientChunkEntities();

			inline void EnableCollisionGeneration(bool enable);

			ClientChunkEntities& operator=(const ClientChunkEntities&) = delete;
			ClientChunkEntities& operator=(ClientChunkEntities&&) = delete;

		private:
			struct ColliderModelUpdateJob : UpdateJob
			{
				Chunk::ColliderData collider;
				std::shared_ptr<Nz::GraphicalMesh> mesh;
			};

			std::shared_ptr<Nz::Mesh> BuildMesh(const Chunk& chunk);
			ColliderModelUpdateJob* ProcessChunkUpdate(const Chunk& chunk, NeighborChunkMask neighborMask) override;
			void UpdateChunkDebugCollider(entt::handle chunkEntity);

			std::mutex m_asyncTransferMutex;
			std::shared_ptr<Nz::MaterialInstance> m_chunkMaterial;
			std::shared_ptr<Nz::MaterialInstance> m_debugColliderMat;
			std::shared_ptr<Nz::VertexDeclaration> m_chunkVertexDeclaration;
			Nz::Signal<double>::ConnectionGuard m_onChunkNormalSmoothAngleUpdatedSlot;
			ConfigFile& m_configFile;
			bool m_isCollisionGenerationEnabled;
	};
}

#include <ClientLib/ClientChunkEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
