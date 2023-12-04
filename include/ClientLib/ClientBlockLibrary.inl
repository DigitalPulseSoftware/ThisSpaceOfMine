// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

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
}

