// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTASSETLIBRARYAPPCOMPONENT_HPP
#define TSOM_CLIENTLIB_CLIENTASSETLIBRARYAPPCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <Nazara/Core/ObjectLibrary.hpp>

namespace Nz
{
	class Font;
	class Model;
	class TextureAsset;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientAssetLibraryAppComponent : public Nz::ApplicationComponent
	{
		public:
			using ApplicationComponent::ApplicationComponent;
			ClientAssetLibraryAppComponent(const ClientAssetLibraryAppComponent&) = delete;
			ClientAssetLibraryAppComponent(ClientAssetLibraryAppComponent&&) = delete;
			~ClientAssetLibraryAppComponent() = default;

			inline std::shared_ptr<Nz::Font> GetFont(std::string_view name) const;
			inline std::shared_ptr<Nz::Model> GetModel(std::string_view name) const;
			inline std::shared_ptr<Nz::TextureAsset> GetTexture(std::string_view name) const;

			inline void RegisterFont(std::string name, std::shared_ptr<Nz::Font> font);
			inline void RegisterModel(std::string name, std::shared_ptr<Nz::Model> font);
			inline void RegisterTexture(std::string name, std::shared_ptr<Nz::TextureAsset> font);

			ClientAssetLibraryAppComponent& operator=(const ClientAssetLibraryAppComponent&) = delete;
			ClientAssetLibraryAppComponent& operator=(ClientAssetLibraryAppComponent&&) = delete;

		private:
			Nz::ObjectLibrary<Nz::Font> m_fontLibrary;
			Nz::ObjectLibrary<Nz::Model> m_modelLibrary;
			Nz::ObjectLibrary<Nz::TextureAsset> m_textureLibrary;
	};
}

#include <ClientLib/ClientAssetLibraryAppComponent.inl>

#endif // TSOM_CLIENTLIB_CLIENTASSETLIBRARYAPPCOMPONENT_HPP
