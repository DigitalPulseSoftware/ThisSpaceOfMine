// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <NazaraUtils/Algorithm.hpp>
#include <fmt/format.h>
#include <frozen/string.h>
#include <frozen/unordered_map.h>
#include <nlohmann/json.hpp>

namespace tsom
{
	namespace
	{
		static constexpr auto s_objectLayers = frozen::make_unordered_map<frozen::string, Nz::UInt16>({
			{ "Dynamic", tsom::Constants::ObjectLayerDynamic },
			{ "DynamicNoCollision", tsom::Constants::ObjectLayerDynamicNoCollision },
			{ "DynamicNoPlayer", tsom::Constants::ObjectLayerDynamicNoPlayer },
			{ "DynamicTrigger", tsom::Constants::ObjectLayerDynamicTrigger },
			{ "Player", tsom::Constants::ObjectLayerPlayer },
			{ "PlayerOnlyTrigger", tsom::Constants::ObjectLayerPlayerOnlyTrigger },
			{ "Static", tsom::Constants::ObjectLayerStatic },
			{ "StaticNoplayer", tsom::Constants::ObjectLayerStaticNoPlayer },
			{ "StaticTrigger", tsom::Constants::ObjectLayerStaticTrigger },
			{ "StaticWater", tsom::Constants::ObjectLayerStaticWater }
		});
	}
}

namespace nlohmann
{
	template<typename BasicJsonType>
	void from_json(const BasicJsonType& j, tsom::BlockLibrary::PhysicsLayer& layerContainer)
	{
		const std::string& layerName = j;

		if (auto it = tsom::s_objectLayers.find(frozen::string(layerName)); it != tsom::s_objectLayers.end())
			layerContainer.layer = it->second;
		else
			throw std::runtime_error(fmt::format("invalid physics layer \"{}\"", layerName));
	}

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(tsom::BlockLibrary::BlockInfo, \
		layerName, basePath, hasCollisions, isDoubleSided, \
		isSmooth, isTransparent, density, \
		metalness, permeability, roughness \
	);
	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(tsom::BlockLibrary::LayerInfo, \
		physicsLayer, isBlended, isFluid, isPhysicsTrigger, renderLayer
	);
}

namespace tsom
{
	bool BlockLibrary::LoadFromString(std::string_view content, bool merge)
	{
		nlohmann::ordered_json doc = nlohmann::ordered_json::parse(content);

		if (!merge)
			Clear();

		for (const auto& [layerName, layerEntryDoc] : doc["layers"].items())
			RegisterLayer(layerName, layerEntryDoc);

		for (const auto& [blockName, blockEntryDoc] : doc["blocks"].items())
			RegisterBlock(blockName, blockEntryDoc);

		return true;
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
		layerData.physicsLayer = layerInfo.physicsLayer.layer;
		layerData.renderLayer = layerInfo.renderLayer;

		assert(!m_layerIndices.contains(name));
		m_layerIndices.emplace(std::move(name), layerIndex);

		return layerIndex;
	}
}
