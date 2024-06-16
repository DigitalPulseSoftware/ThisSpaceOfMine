// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP
#define TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/BlockLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
	class RenderDevice;
	class TextureAsset;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientBlockLibrary : public BlockLibrary
	{
		public:
			inline ClientBlockLibrary(Nz::ApplicationBase& applicationBase);
			~ClientBlockLibrary() = default;

			void BuildTexture();

			inline const std::shared_ptr<Nz::TextureAsset>& GetBaseColorTexture() const;
			inline const std::shared_ptr<Nz::TextureAsset>& GetDetailTexture() const;
			inline const std::shared_ptr<Nz::TextureAsset>& GetNormalTexture() const;
			inline const std::shared_ptr<Nz::TextureAsset>& GetPreviewTexture(BlockIndex blockIndex) const;

		private:
			std::shared_ptr<Nz::TextureAsset> m_baseColorTexture;
			std::shared_ptr<Nz::TextureAsset> m_detailTexture;
			std::shared_ptr<Nz::TextureAsset> m_normalTexture;
			std::vector<std::shared_ptr<Nz::TextureAsset>> m_previewTextures;
			Nz::ApplicationBase& m_applicationBase;
	};
}

#include <ClientLib/ClientBlockLibrary.inl>

#endif // TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP
