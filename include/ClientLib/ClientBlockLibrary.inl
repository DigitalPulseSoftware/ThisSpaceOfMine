// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ClientBlockLibrary::ClientBlockLibrary(Nz::ApplicationBase& applicationBase) :
	m_applicationBase(applicationBase),
	m_globalBlockBufferPtr(nullptr)
	{
		// HAAAAAAAAAAAAAAAAAAAAX
		// Re-enable collisions for forcefield to allow clients to remove them (player collisions are only handled on the server for now)
		if (BlockIndex idx = GetBlockIndex("forcefield"); idx != InvalidBlockIndex)
			m_blocks[idx].hasCollisions = true;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetBlockTexture(ClientAssetCookRegistry::TextureType textureType) const
	{
		return m_blockTextures[textureType];
	}

	inline const std::shared_ptr<Nz::RenderBuffer>& ClientBlockLibrary::GetGlobalBlockBuffer() const
	{
		return m_globalBlockBuffer;
	}

	inline const std::shared_ptr<Nz::TextureAsset>& ClientBlockLibrary::GetPreviewTexture(BlockIndex blockIndex) const
	{
		assert(blockIndex < m_previewTextures.size());
		return m_previewTextures[blockIndex];
	}
}
