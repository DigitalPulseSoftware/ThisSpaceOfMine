// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ClientBlockLibrary::ClientBlockLibrary(Nz::ApplicationBase& applicationBase, Nz::RenderDevice& renderDevice) :
	m_applicationBase(applicationBase),
	m_renderDevice(renderDevice)
	{
	}

	inline const std::shared_ptr<Nz::Texture>& ClientBlockLibrary::GetBaseColorTexture() const
	{
		return m_baseColorTexture;
	}

	inline const std::shared_ptr<Nz::Texture>& ClientBlockLibrary::GetDetailTexture() const
	{
		return m_detailTexture;
	}

	inline const std::shared_ptr<Nz::Texture>& ClientBlockLibrary::GetNormalTexture() const
	{
		return m_normalTexture;
	}

	inline const std::shared_ptr<Nz::Texture>& ClientBlockLibrary::GetPreviewTexture(BlockIndex blockIndex) const
	{
		assert(blockIndex < m_previewTextures.size());
		return m_previewTextures[blockIndex];
	}
}
