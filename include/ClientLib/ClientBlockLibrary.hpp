// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP
#define TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/CookedBlockRegistry.hpp>

namespace Nz
{
	class ApplicationBase;
	class GpuBuffer;
	class GpuDevice;
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

			void BuildTexture(Nz::GpuDevice& gpuDevice);

			inline const std::shared_ptr<Nz::TextureAsset>& GetBlockTexture(CookedBlockRegistry::TextureType textureType) const;
			inline const std::shared_ptr<Nz::GpuBuffer>& GetGlobalBlockBuffer() const;
			inline const std::shared_ptr<Nz::TextureAsset>& GetPreviewTexture(BlockIndex blockIndex) const;

			bool LoadFromString(std::string_view content, bool merge = false) override;

		private:
			std::shared_ptr<Nz::GpuBuffer> m_globalBlockBuffer;
			std::vector<std::shared_ptr<Nz::TextureAsset>> m_previewTextures;
			Nz::EnumArray<CookedBlockRegistry::TextureType, std::shared_ptr<Nz::TextureAsset>> m_blockTextures;
			Nz::ApplicationBase& m_applicationBase;
			void* m_globalBlockBufferPtr;
	};
}

#include <ClientLib/ClientBlockLibrary.inl>

#endif // TSOM_CLIENTLIB_CLIENTBLOCKLIBRARY_HPP
