// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline auto BlockLibrary::GetBlockData(BlockIndex blockIndex) const -> const BlockData&
	{
		return m_blocks[blockIndex];
	}

	inline auto BlockLibrary::GetLayerData(std::size_t layerIndex) const -> const LayerData&
	{
		return m_layers[layerIndex];
	}

	inline BlockIndex BlockLibrary::GetBlockIndex(std::string_view blockName) const
	{
		auto it = m_blockIndices.find(blockName);
		if (it == m_blockIndices.end())
			return InvalidBlockIndex;

		return it->second;
	}

	inline bool BlockLibrary::IsValidBlock(BlockIndex blockIndex) const
	{
		return blockIndex < m_blocks.size();
	}

	inline bool BlockLibrary::IsValidLayer(std::size_t layerIndex) const
	{
		return layerIndex < m_layers.size();
	}
}
