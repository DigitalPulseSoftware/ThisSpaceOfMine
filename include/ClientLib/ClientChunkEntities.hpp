// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP
#define TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP

#include <CommonLib/ChunkEntities.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <Nazara/Core/Color.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
	class MaterialInstance;
	class Model;
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
			std::shared_ptr<Nz::Model> BuildModel(const Chunk* chunk);
			void CreateChunkEntity(std::size_t chunkId, const Chunk* chunk) override;
			void UpdateChunkEntity(std::size_t chunkId) override;
			void UpdateChunkDebugCollider(std::size_t chunkId);

			std::shared_ptr<Nz::MaterialInstance> m_chunkMaterial;
			std::shared_ptr<Nz::VertexDeclaration> m_chunkVertexDeclaration;
	};
}

#include <ClientLib/ClientChunkEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP
