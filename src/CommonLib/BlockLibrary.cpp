// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/BlockLibrary.hpp>
#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	BlockLibrary::BlockLibrary()
	{
		RegisterBlock("empty", {
			.hasCollisions = false,
			.permeability = 1.f
		});

		RegisterBlock("debug", {
			.baseBackPath = "blocks/debug_back",
			.baseDownPath = "blocks/debug_down",
			.baseFrontPath = "blocks/debug_front",
			.baseLeftPath = "blocks/debug_left",
			.baseRightPath = "blocks/debug_right",
			.baseUpPath = "blocks/debug_up",
		});

		RegisterBlock("dirt", {
			.basePath = "blocks/dirt",
			.permeability = 0.1f
		});

		RegisterBlock("grass", {
			.basePath = "blocks/grass_top",
			.baseDownPath = "blocks/dirt",
			.baseSidePath = "blocks/grass_side",
			.permeability = 0.1f
		});

		RegisterBlock("hull", {
			.basePath = "blocks/smooth_stone"
		});

		RegisterBlock("hull2", {
			.basePath = "blocks/smooth_stone_slab_side"
		});

		RegisterBlock("snow", {
			.basePath = "blocks/snow",
			.permeability = 0.5f
		});

		RegisterBlock("stone", {
			.basePath = "blocks/cobblestone"
		});

		RegisterBlock("stone_mossy", {
			.basePath = "blocks/mossy_cobblestone"
		});

		RegisterBlock("forcefield", {
			.basePath = "blocks/frosted_ice_0",
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
	}

	BlockIndex BlockLibrary::RegisterBlock(std::string name, BlockInfo blockInfo)
	{
		BlockIndex blockIndex = Nz::SafeCast<BlockIndex>(m_blocks.size());

		auto& blockData = m_blocks.emplace_back();
		blockData.hasCollisions = blockInfo.hasCollisions;
		blockData.permeability = blockInfo.permeability;
		blockData.name = name;

		unsigned int baseTexIndex;
		if (!blockInfo.basePath.empty())
			baseTexIndex = RegisterTexture(std::move(blockInfo.basePath));
		else
			baseTexIndex = 0;

		unsigned int baseSideTexIndex;
		if (!blockInfo.baseSidePath.empty())
			baseSideTexIndex = RegisterTexture(std::move(blockInfo.baseSidePath));
		else
			baseSideTexIndex = baseTexIndex;

		const Nz::EnumArray<Direction, std::string*> dirToStr = {
			&blockInfo.baseBackPath,  //< Back
			&blockInfo.baseDownPath,  //< Down
			&blockInfo.baseFrontPath, //< Front
			&blockInfo.baseLeftPath,  //< Left
			&blockInfo.baseRightPath, //< Right
			&blockInfo.baseUpPath,    //< Up
		};

		for (auto&& [dir, str] : dirToStr.iter_kv())
		{
			if (!str->empty())
				blockData.texIndices[dir] = RegisterTexture(std::move(*str));
			else if (dir != Direction::Up && dir != Direction::Down)
				blockData.texIndices[dir] = baseSideTexIndex;
			else
				blockData.texIndices[dir] = baseTexIndex;
		}

		m_blockIndices.emplace(std::move(name), blockIndex);

		return blockIndex;
	}

	unsigned int BlockLibrary::RegisterTexture(std::string&& texturePath)
	{
		auto it = m_textureIndices.find(texturePath);
		if (it != m_textureIndices.end())
			return it->second;

		unsigned int texIndex = Nz::SafeCast<unsigned int>(m_textureIndices.size() + 1); // Keep slice #0 for empty
		m_textureIndices.emplace(std::move(texturePath), texIndex);

		return texIndex;
	}
}
