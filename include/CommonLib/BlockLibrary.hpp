// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_BLOCKLIBRARY_HPP
#define TSOM_COMMONLIB_BLOCKLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/PhysicsConstants.hpp>
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
			struct LayerData;
			struct LayerInfo;

			BlockLibrary() = default;
			BlockLibrary(const BlockLibrary&) = delete;
			BlockLibrary(BlockLibrary&&) = delete;
			~BlockLibrary() = default;

			void Clear();

			inline const BlockData& GetBlockData(BlockIndex blockIndex) const;
			inline const std::vector<BlockData>& GetBlocks() const;
			inline const LayerData& GetLayerData(std::size_t layerIndex) const;
			inline BlockIndex GetBlockIndex(std::string_view blockName) const;
			inline bool IsValidBlock(BlockIndex blockIndex) const;
			inline bool IsValidLayer(std::size_t layerIndex) const;

			bool LoadFromString(std::string_view content, bool merge = false);

			BlockIndex RegisterBlock(std::string name, BlockInfo blockInfo);
			std::size_t RegisterLayer(std::string name, LayerInfo layerInfo);

			struct BlockInfo
			{
				std::string_view layerName = "default";
				std::string basePath;
				bool hasCollisions = true;
				bool isDoubleSided = false;
				bool isSmooth = false;
				bool isTransparent = false;
				float density = 1.0f;
				float metalness = 0.0f; //< Used if texture is not available
				float permeability = 0.f;
				float roughness = 1.0f; //< Used if texture is not available
			};

			struct BlockData
			{
				std::size_t layerIndex;
				std::string name;
				std::string basePath;
				bool hasCollisions;
				bool isDoubleSided;
				bool isTransparent;
				bool isSmooth;
				float density;
				float metalness = 0.0f; //< Used if texture is not available
				float permeability;
				float roughness = 1.0f; //< Used if texture is not available
			};

			struct PhysicsLayer
			{
				Nz::PhysObjectLayer3D layer;
			};

			struct LayerInfo
			{
				PhysicsLayer physicsLayer = PhysicsLayer { Constants::ObjectLayerStatic };
				bool isBlended;
				bool isFluid = false;
				bool isPhysicsTrigger = false;
				int renderLayer = 0;
			};

			struct LayerData
			{
				std::string name;
				Nz::PhysObjectLayer3D physicsLayer;
				bool isBlended;
				bool isFluid;
				bool isPhysicsTrigger;
				int renderLayer;
			};

			BlockLibrary& operator=(const BlockLibrary&) = delete;
			BlockLibrary& operator=(BlockLibrary&&) = delete;

		protected:
			tsl::hopscotch_map<std::string /*name*/, BlockIndex /*blockIndex*/, std::hash<std::string_view>, std::equal_to<>> m_blockIndices;
			tsl::hopscotch_map<std::string /*name*/, std::size_t /*layerIndex*/, std::hash<std::string_view>, std::equal_to<>> m_layerIndices;
			std::vector<BlockData> m_blocks;
			std::vector<LayerData> m_layers;
	};
}

#include <CommonLib/BlockLibrary.inl>

#endif // TSOM_COMMONLIB_BLOCKLIBRARY_HPP
