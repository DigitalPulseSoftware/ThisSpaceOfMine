// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void ClientChunkEntities::EnableCollisionGeneration(bool enable)
	{
		m_isCollisionGenerationEnabled = enable;
		RebuildAllChunks();
	}

	inline const Nz::GraphicalMesh* ClientChunkEntities::GetChunkMesh(const ChunkIndices& chunkIndices) const
	{
		auto it = m_chunkGraphicsMesh.find(chunkIndices);
		if (it == m_chunkGraphicsMesh.end())
			return nullptr;

		return it->second.get();
	}

	inline const std::shared_ptr<Nz::MaterialInstance>& ClientChunkEntities::GetMaterial() const
	{
		return m_chunkMaterial;
	}

	inline const std::shared_ptr<Nz::VertexDeclaration>& ClientChunkEntities::GetVertexDeclaration() const
	{
		return m_chunkVertexDeclaration;
	}
}
