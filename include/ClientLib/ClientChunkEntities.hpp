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
	class GraphicalMesh;
	class MaterialInstance;
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
			ClientChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary, std::size_t layerIndex);
			ClientChunkEntities(const ClientChunkEntities&) = delete;
			ClientChunkEntities(ClientChunkEntities&&) = delete;
			~ClientChunkEntities() = default;

			inline void EnableCollisionGeneration(bool enable);

			ClientChunkEntities& operator=(const ClientChunkEntities&) = delete;
			ClientChunkEntities& operator=(ClientChunkEntities&&) = delete;

		private:
			struct VoxelBuffer
			{
				std::size_t faceCount;
				std::vector<std::uint8_t> bufferData;
			};

			struct ColliderModelUpdateJob : UpdateJob
			{
				std::shared_ptr<Nz::Collider3D> collider;
				VoxelBuffer voxelData;
			};

			VoxelBuffer BuildMeshData(const Chunk& chunk);
			ColliderModelUpdateJob* ProcessChunkUpdate(const Chunk& chunk, DirectionMask neighborMask) override;
			void UpdateChunkDebugCollider(const ChunkIndices& chunkIndices);

			std::shared_ptr<Nz::GraphicalMesh> m_chunkGraphicalMesh;
			std::shared_ptr<Nz::MaterialInstance> m_chunkReferenceMaterial;
			bool m_isCollisionGenerationEnabled;
			bool m_shouldDrawDebugColliders;
	};
}

#include <ClientLib/ClientChunkEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTCHUNKENTITIES_HPP
