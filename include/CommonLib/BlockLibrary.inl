// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline auto BlockLibrary::GetBlockData(BlockIndex blockIndex) const -> const BlockData&
	{
		return m_blocks[blockIndex];
	}

	inline BlockIndex BlockLibrary::GetBlockIndex(std::string_view blockName) const
	{
		auto it = m_blockIndices.find(blockName);
		if (it == m_blockIndices.end())
			return InvalidBlockIndex;

		return it->second;
	}
}
