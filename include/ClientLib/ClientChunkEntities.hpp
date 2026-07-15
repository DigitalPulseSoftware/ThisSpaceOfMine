// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
#define TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP

#include <ClientLib/ClientBlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <Nazara/Core/Color.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class ApplicationBase;
	class AsyncRenderCommands;
	class EnttWorld;
	class MaterialInstance;
	class Mesh;
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
			ClientChunkEntities(Nz::ApplicationBase& app, ConfigFile& config, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary, std::size_t layerIndex);
			ClientChunkEntities(const ClientChunkEntities&) = delete;
			ClientChunkEntities(ClientChunkEntities&&) = delete;
			~ClientChunkEntities();

			inline void EnableCollisionGeneration(bool enable);

			void Update() override;

			ClientChunkEntities& operator=(const ClientChunkEntities&) = delete;
			ClientChunkEntities& operator=(ClientChunkEntities&&) = delete;

		private:
			struct ColliderModelUpdateJob : UpdateJob
			{
				std::shared_ptr<Nz::Collider3D> collider;
				std::shared_ptr<Nz::Mesh> mesh;
			};

			std::shared_ptr<Nz::Mesh> BuildMesh(const Chunk& chunk);
			ColliderModelUpdateJob* ProcessChunkUpdate(const Chunk& chunk, NeighborChunkMask neighborMask) override;
			void UpdateChunkDebugCollider(const ChunkIndices& chunkIndices);

			std::shared_ptr<Nz::MaterialInstance> m_chunkMaterial;
			std::shared_ptr<Nz::VertexDeclaration> m_chunkVertexDeclaration;
			std::unique_ptr<Nz::AsyncRenderCommands> m_asyncTransfer;
			Nz::Signal<double>::ConnectionGuard m_onChunkNormalSmoothAngleUpdatedSlot;
			ConfigFile& m_configFile;
			bool m_isCollisionGenerationEnabled;
	};
}

#include <ClientLib/ClientChunkEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
