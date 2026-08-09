// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
	class Material;
	class MaterialInstance;
	class Model;
	class TextureAsset;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientAssetLibraryAppComponent : public Nz::ApplicationComponent
	{
		public:
			ClientAssetLibraryAppComponent(Nz::ApplicationBase& app);
			ClientAssetLibraryAppComponent(const ClientAssetLibraryAppComponent&) = delete;
			ClientAssetLibraryAppComponent(ClientAssetLibraryAppComponent&&) = delete;
			~ClientAssetLibraryAppComponent() = default;

			bool CheckAssets();

			inline std::shared_ptr<Nz::Font> GetFont(std::string_view name) const;
			inline std::shared_ptr<Nz::Material> GetMaterial(std::string_view name) const;
			inline std::shared_ptr<Nz::MaterialInstance> GetMaterialInstance(std::string_view name) const;
			inline std::shared_ptr<Nz::Model> GetModel(std::string_view name) const;
			inline std::shared_ptr<Nz::TextureAsset> GetTexture(std::string_view name) const;

			inline std::shared_ptr<Nz::Font> QueryFont(std::string_view name) const;
			inline std::shared_ptr<Nz::Material> QueryMaterial(std::string_view name) const;
			inline std::shared_ptr<Nz::MaterialInstance> QueryMaterialInstance(std::string_view name) const;
			inline std::shared_ptr<Nz::Model> QueryModel(std::string_view name) const;
			inline std::shared_ptr<Nz::TextureAsset> QueryTexture(std::string_view name) const;

			inline void RegisterFont(std::string name, std::shared_ptr<Nz::Font> font);
			inline void RegisterMaterial(std::string name, std::shared_ptr<Nz::Material> material);
			inline void RegisterMaterialInstance(std::string name, std::shared_ptr<Nz::MaterialInstance> materialInstance);
			inline void RegisterModel(std::string name, std::shared_ptr<Nz::Model> font);
			inline void RegisterTexture(std::string name, std::shared_ptr<Nz::TextureAsset> font);

			ClientAssetLibraryAppComponent& operator=(const ClientAssetLibraryAppComponent&) = delete;
			ClientAssetLibraryAppComponent& operator=(ClientAssetLibraryAppComponent&&) = delete;

		private:
			void RegisterPBRMaterial();

			Nz::ObjectLibrary<Nz::Font> m_fontLibrary;
			Nz::ObjectLibrary<Nz::Material> m_materialLibrary;
			Nz::ObjectLibrary<Nz::MaterialInstance> m_materialInstanceLibrary;
			Nz::ObjectLibrary<Nz::Model> m_modelLibrary;
			Nz::ObjectLibrary<Nz::TextureAsset> m_textureLibrary;
	};
}

#include <ClientLib/ClientAssetLibraryAppComponent.inl>

#endif // TSOM_CLIENTLIB_CLIENTASSETLIBRARYAPPCOMPONENT_HPP
