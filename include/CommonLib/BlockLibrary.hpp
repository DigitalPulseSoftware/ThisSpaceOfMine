// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_BLOCKLIBRARY_HPP
#define TSOM_COMMONLIB_BLOCKLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Direction.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <tsl/hopscotch_map.h>
#include <string>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API BlockLibrary
	{
		public:
			struct BlockData;
			struct BlockInfo;

			BlockLibrary();
			BlockLibrary(const BlockLibrary&) = delete;
			BlockLibrary(BlockLibrary&&) = delete;
			~BlockLibrary() = default;

			inline const BlockData& GetBlockData(BlockIndex blockIndex) const;
			inline BlockIndex GetBlockIndex(std::string_view blockName) const;

			BlockIndex RegisterBlock(std::string name, BlockInfo blockInfo);

			struct BlockInfo
			{
				std::string basePath;
				std::string baseBackPath;
				std::string baseDownPath;
				std::string baseFrontPath;
				std::string baseLeftPath;
				std::string baseRightPath;
				std::string baseSidePath;
				std::string baseUpPath;
				bool hasCollisions = true;
				float permeability = 0.f;
			};

			struct BlockData
			{
				std::string name;
				Nz::EnumArray<Direction, unsigned int> texIndices;
				bool hasCollisions;
				float permeability;
			};

			BlockLibrary& operator=(const BlockLibrary&) = delete;
			BlockLibrary& operator=(BlockLibrary&&) = delete;

		private:
			unsigned int RegisterTexture(std::string&& texturePath);

		protected:
			tsl::hopscotch_map<std::string /*name*/, BlockIndex /*blockIndex*/, std::hash<std::string_view>, std::equal_to<>> m_blockIndices;
			tsl::hopscotch_map<std::string /*texturePath*/, unsigned int /*textureIndex*/, std::hash<std::string_view>, std::equal_to<>> m_textureIndices;
			std::vector<BlockData> m_blocks;
	};
}

#include <CommonLib/BlockLibrary.inl>

#endif // TSOM_COMMONLIB_BLOCKLIBRARY_HPP
