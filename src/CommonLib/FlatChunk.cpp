// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <fmt/format.h>

namespace tsom
{
	std::shared_ptr<Nz::Collider3D> FlatChunk::BuildCollider(const BlockLibrary& /*blockManager*/) const
	{
		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;

		Nz::Bitset<Nz::UInt64> availableBlocks = GetCollisionCellMask();

		std::optional<Nz::Vector3ui> startPos;
		auto CommitCollider = [&](unsigned int endX)
		{
			if (!startPos)
				return;

			unsigned int endY = startPos->y;
			unsigned int endZ = startPos->z;

			// Try to grow on Y
			for (endY += 1; endY < m_size.y; ++endY)
			{
				auto CheckX = [&]
				{
					for (unsigned int x = startPos->x; x <= endX; ++x)
					{
						if (!availableBlocks[GetBlockLocalIndex({ x, endY, endZ })])
							return false;
					}

					return true;
				};

				if (!CheckX())
					break;
			}
			endY--;

			// Try to grow on Z
			for (endZ += 1; endZ < m_size.z; ++endZ)
			{
				auto CheckXY = [&]
				{
					for (unsigned int y = startPos->y; y <= endY; ++y)
					{
						for (unsigned int x = startPos->x; x <= endX; ++x)
						{
							if (!availableBlocks[GetBlockLocalIndex({ x, y, endZ })])
								return false;
						}
					}

					return true;
				};

				if (!CheckXY())
					break;
			}
			endZ--;

			for (unsigned int z = startPos->z; z <= endZ; ++z)
			{
				for (unsigned int y = startPos->y; y <= endY; ++y)
				{
					for (unsigned int x = startPos->x; x <= endX; ++x)
						availableBlocks[GetBlockLocalIndex({ x, y, z })] = false;
				}
			}

			Nz::Vector3f startOffset(startPos->x, startPos->z, startPos->y);
			Nz::Vector3f endOffset(endX + 1, endZ + 1, endY + 1);

			Nz::Vector3f size = (endOffset - startOffset) * m_blockSize;

			auto& childCollider = childColliders.emplace_back();
			childCollider.offset = startOffset * m_blockSize + size * 0.5f - Nz::Vector3f(m_size) * m_blockSize * 0.5f;
			childCollider.collider = std::make_shared<Nz::BoxCollider3D>(size);

			startPos.reset();
		};

		Nz::Vector3f offset = Nz::Vector3f(m_blockSize) * 0.5f;
		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					if (!availableBlocks[GetBlockLocalIndex({ x, y, z })])
					{
						CommitCollider(x - 1);
						continue;
					}

					if (!startPos)
						startPos = { x, y, z };
				}
				CommitCollider(m_size.x - 1);
			}
		}

		if (childColliders.empty())
			return {};

		return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	std::optional<Nz::Vector3ui> FlatChunk::ComputeCoordinates(const Nz::Vector3f& position) const
	{
		Nz::Vector3f indices = position - m_owner.GetChunkOffset(m_indices);
		indices += Nz::Vector3f(m_size) * m_blockSize * 0.5f;
		indices /= m_blockSize;

		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_size.x || pos.y >= m_size.y || pos.z >= m_size.z)
			return std::nullopt;

		return pos;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> FlatChunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize;

		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		return box.GetCorners();
	}
}
