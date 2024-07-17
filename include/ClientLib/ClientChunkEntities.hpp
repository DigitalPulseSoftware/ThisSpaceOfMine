// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
		Nz::Vector3f uvw;
		Nz::Vector3f tangent;
	};

	class TSOM_CLIENTLIB_API ClientChunkEntities final : public ChunkEntities
	{
		public:
			ClientChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary);
			ClientChunkEntities(const ClientChunkEntities&) = delete;
			ClientChunkEntities(ClientChunkEntities&&) = delete;
			~ClientChunkEntities() = default;

			ClientChunkEntities& operator=(const ClientChunkEntities&) = delete;
			ClientChunkEntities& operator=(ClientChunkEntities&&) = delete;

		private:
			struct ColliderModelUpdateJob : UpdateJob
			{
				std::shared_ptr<Nz::Collider3D> collider;
				std::shared_ptr<Nz::Mesh> mesh;
			};

			std::shared_ptr<Nz::Mesh> BuildMesh(const Chunk* chunk);
			void HandleChunkUpdate(const ChunkIndices& chunkIndices, const Chunk* chunk) override;
			void UpdateChunkDebugCollider(const ChunkIndices& chunkIndices);

			std::shared_ptr<Nz::MaterialInstance> m_chunkMaterial;
			std::shared_ptr<Nz::VertexDeclaration> m_chunkVertexDeclaration;
	};
}

#include <ClientLib/ClientChunkEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
