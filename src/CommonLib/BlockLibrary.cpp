// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/BlockLibrary.hpp>
#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	BlockLibrary::BlockLibrary()
	{
		/************************************************************************/
		RegisterLayer("default", {
			.isBlended = false
		});

		RegisterLayer("water", {
			.physicsLayer = Constants::ObjectLayerStaticWater,
			.isBlended = true,
			.isFluid = true,
			.isPhysicsTrigger = true,
			.renderLayer = 100
		});

		/************************************************************************/
		RegisterBlock("empty", {
			.hasCollisions = false,
			.isSmooth = true,
			.isTransparent = true,
			.permeability = 1.f
		});

		RegisterBlock("debug", {
			.basePath = "blocks/debug_up",
		});

		RegisterBlock("dirt", {
			.basePath = "blocks/dirt",
			.isSmooth = true,
			.density = 1.0f,
			.permeability = 0.1f
		});

		RegisterBlock("grass", {
			.basePath = "blocks/grass",
			.isSmooth = true,
			.density = 2.0f,
			.permeability = 0.1f
		});

		RegisterBlock("hull", {
			.basePath = "blocks/smooth_stone"
		});

		RegisterBlock("snow", {
			.basePath = "blocks/snow",
			.isSmooth = true,
			.permeability = 0.5f
		});

		RegisterBlock("stone", {
			.basePath = "blocks/cobblestone",
			.isSmooth = true,
			.density = 4.0f
		});

		RegisterBlock("stone_mossy", {
			.basePath = "blocks/mossy_cobblestone",
			.isSmooth = true,
		});

		RegisterBlock("forcefield", {
			.basePath = "blocks/forcefield",
			.hasCollisions = false,
			.isTransparent = true
		});

		RegisterBlock("planks", {
			.basePath = "blocks/planks",
		});

		RegisterBlock("stone_bricks", {
			.basePath = "blocks/stone_bricks",
		});

		RegisterBlock("copper_block", {
			.basePath = "blocks/copper_block",
		});

		RegisterBlock("glass", {
			.basePath = "blocks/glass",
			.isDoubleSided = true,
			.isSmooth = true,
			.isTransparent = true
		});

		RegisterBlock("water", {
			.layerName = "water",
			.basePath = "blocks/water",
			.isDoubleSided = true,
			.isSmooth = true,
			.isTransparent = true,
		});

		RegisterBlock("bark", {
			.basePath = "blocks/bark",
			.isSmooth = true
		});

		RegisterBlock("cliff_rocks", {
			.basePath = "blocks/cliff_rocks",
			.isSmooth = true
		});

		RegisterBlock("rock", {
			.basePath = "blocks/rock",
			.isSmooth = true
		});

		RegisterBlock("wood_floor", {
			.basePath = "blocks/wood_floor",
		});

		RegisterBlock("white_bricks", {
			.basePath = "blocks/white_bricks",
		});

		RegisterBlock("gold", {
			.basePath = "blocks/gold",
		});

		RegisterBlock("metal", {
			.basePath = "blocks/metal",
		});

		RegisterBlock("metal_plates", {
			.basePath = "blocks/metal_plates",
		});

		RegisterBlock("brickswall", {
			.basePath = "blocks/brickswall",
		});

		RegisterBlock("floor_tiles", {
			.basePath = "blocks/floor_tiles",
		});
	}

	BlockIndex BlockLibrary::RegisterBlock(std::string name, BlockInfo blockInfo)
	{
		BlockIndex blockIndex = Nz::SafeCast<BlockIndex>(m_blocks.size());

		auto& blockData = m_blocks.emplace_back();
		blockData.hasCollisions = blockInfo.hasCollisions;
		blockData.isDoubleSided = blockInfo.isDoubleSided;
		blockData.isTransparent = blockInfo.isTransparent;
		blockData.isSmooth = blockInfo.isSmooth;
		blockData.density = blockInfo.density;
		blockData.metalness = blockInfo.metalness;
		blockData.permeability = blockInfo.permeability;
		blockData.roughness = blockInfo.roughness;
		blockData.name = name;
		blockData.basePath = std::move(blockInfo.basePath);

		auto it = m_layerIndices.find(blockInfo.layerName);
		NazaraAssertMsg(it != m_layerIndices.end(), "Invalid layer %s", blockInfo.layerName.data());
		blockData.layerIndex = Nz::SafeCaster(it->second);

		assert(!m_blockIndices.contains(name));
		m_blockIndices.emplace(std::move(name), blockIndex);

		return blockIndex;
	}

	std::size_t BlockLibrary::RegisterLayer(std::string name, LayerInfo layerInfo)
	{
		std::size_t layerIndex = m_layers.size();

		auto& layerData = m_layers.emplace_back();
		layerData.isBlended = layerInfo.isBlended;
		layerData.isFluid = layerInfo.isFluid;
		layerData.isPhysicsTrigger = layerInfo.isPhysicsTrigger;
		layerData.name = name;
		layerData.physicsLayer = layerInfo.physicsLayer;
		layerData.renderLayer = layerInfo.renderLayer;

		assert(!m_layerIndices.contains(name));
		m_layerIndices.emplace(std::move(name), layerIndex);

		return layerIndex;
	}
}
