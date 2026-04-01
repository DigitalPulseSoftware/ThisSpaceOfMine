// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
	class RenderBuffer;
	class RenderDevice;
	class Texture;
	class TextureAsset;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientBlockLibrary : public BlockLibrary
	{
		public:
			inline ClientBlockLibrary(Nz::ApplicationBase& applicationBase);
			~ClientBlockLibrary() = default;

			void BuildTexture(Nz::RenderDevice& renderDevice);

			inline const std::shared_ptr<Nz::TextureAsset>& GetBaseColorTexture() const;
			inline const std::shared_ptr<Nz::RenderBuffer>& GetGlobalBlockBuffer() const;
			inline const std::shared_ptr<Nz::TextureAsset>& GetPreviewTexture(BlockIndex blockIndex) const;

		private:
			std::shared_ptr<Nz::RenderBuffer> m_globalBlockBuffer;
			std::shared_ptr<Nz::Texture> m_texture;
			std::shared_ptr<Nz::TextureAsset> m_baseColorTexture;
			std::vector<std::shared_ptr<Nz::TextureAsset>> m_previewTextures;
			Nz::ApplicationBase& m_applicationBase;
			void* m_globalBlockBufferPtr;
	};
}

#include <ClientLib/ClientBlockLibrary.inl>

#endif // TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP
