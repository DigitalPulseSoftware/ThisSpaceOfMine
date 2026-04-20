// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COOKEDBLOCKREGISTRY_HPP
#define TSOM_COMMONLIB_COOKEDBLOCKREGISTRY_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Color.hpp>
#include <NazaraUtils/Result.hpp>
#include <NazaraUtils/StringHash.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace tsom
{
	class TSOM_COMMONLIB_API CookedBlockRegistry
	{
		public:
			struct BlockEntry;

			CookedBlockRegistry() = default;
			CookedBlockRegistry(const CookedBlockRegistry&) = delete;
			CookedBlockRegistry(CookedBlockRegistry&&) = default;
			~CookedBlockRegistry() = default;

			void AddBlock(std::string blockName, BlockEntry blockEntry);

			const BlockEntry& GetBlock(std::string_view blockName) const;

			bool SaveToFile(const std::filesystem::path& path) const;

			CookedBlockRegistry& operator=(const CookedBlockRegistry&) = delete;
			CookedBlockRegistry& operator=(CookedBlockRegistry&&) = default;

			static std::optional<CookedBlockRegistry> LoadFromString(std::string_view content);
			static std::optional<CookedBlockRegistry> LoadFromFile(const std::filesystem::path& path);

			enum class TextureType
			{
				None = -1,
				BC1,
				BC3,
				BC4,
				BC5,

				Max = BC5
			};

			struct Texture
			{
				TextureType type = TextureType::None;
				std::string path;
			};

			struct BlockEntry
			{
				Nz::Color baseColorFallback;
				Texture ambientOcclusionHeightTexture;
				Texture baseColorTexture;
				Texture normalMapTexture;
				Texture roughnessMetalnessTexture;
				float ambientOcclusionFallback;
				float roughnessFallback;
				float metalnessFallback;
			};

		private:
			std::unordered_map<std::string, BlockEntry, Nz::StringHash<>, std::equal_to<>> m_blockEntries;
	};
}

#endif // TSOM_COMMONLIB_COOKEDBLOCKREGISTRY_HPP
