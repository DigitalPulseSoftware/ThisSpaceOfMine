// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline std::shared_ptr<Nz::Font> ClientAssetLibraryAppComponent::GetFont(std::string_view name) const
	{
		return m_fontLibrary.Get(name);
	}

	inline std::shared_ptr<Nz::Model> ClientAssetLibraryAppComponent::GetModel(std::string_view name) const
	{
		return m_modelLibrary.Get(name);
	}

	inline std::shared_ptr<Nz::TextureAsset> ClientAssetLibraryAppComponent::GetTexture(std::string_view name) const
	{
		return m_textureLibrary.Get(name);
	}

	inline void ClientAssetLibraryAppComponent::RegisterFont(std::string name, std::shared_ptr<Nz::Font> font)
	{
		m_fontLibrary.Register(std::move(name), std::move(font));
	}

	inline void ClientAssetLibraryAppComponent::RegisterModel(std::string name, std::shared_ptr<Nz::Model> model)
	{
		m_modelLibrary.Register(std::move(name), std::move(model));
	}

	inline void ClientAssetLibraryAppComponent::RegisterTexture(std::string name, std::shared_ptr<Nz::TextureAsset> texture)
	{
		m_textureLibrary.Register(std::move(name), std::move(texture));
	}
}
