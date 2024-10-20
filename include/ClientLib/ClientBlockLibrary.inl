// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ClientBlockLibrary::ClientBlockLibrary(Nz::ApplicationBase& applicationBase) :
	m_applicationBase(applicationBase)
	{
		// HAAAAAAAAAAAAAAAAAAAAX
		// Re-enable collisions for forcefield to allow clients to remove them (player collisions are only handled on the server for now)
		if (BlockIndex idx = GetBlockIndex("forcefield"); idx != InvalidBlockIndex)
			m_blocks[idx].hasCollisions = true;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetBaseColorTexture() const
	{
		return m_baseColorTexture;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetDetailTexture() const
	{
		return m_detailTexture;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetNormalTexture() const
	{
		return m_normalTexture;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetPreviewTexture(BlockIndex blockIndex) const
	{
		assert(blockIndex < m_previewTextures.size());
		return m_previewTextures[blockIndex];
	}
}
