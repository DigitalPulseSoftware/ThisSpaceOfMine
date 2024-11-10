// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/FlatChunk.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/Bitset.hpp>

namespace tsom
{
	std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> FlatChunk::BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale) const
	{
		Nz::Vector3f offset = (Nz::Vector3f(blockIndices.x, blockIndices.z, blockIndices.y) - Nz::Vector3f(m_size) * 0.5f + Nz::Vector3f(0.5f)) * m_blockSize;
		return { std::make_shared<Nz::BoxCollider3D>(Nz::Vector3f(m_blockSize * scale)), offset };
	}

	std::shared_ptr<Nz::Collider3D> FlatChunk::BuildCollider() const
	{
		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;

		auto AddBox = [&](const Nz::Boxf& box)
		{
			auto& childCollider = childColliders.emplace_back();
			childCollider.offset = box.GetCenter() * m_blockSize;
			childCollider.collider = std::make_shared<Nz::BoxCollider3D>(box.GetLengths() * m_blockSize);
		};

		BuildCollider(m_size, GetCollisionCellMask(), AddBox);

		if (childColliders.empty())
			return nullptr;

		return std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
	}

	std::optional<Nz::Vector3ui> FlatChunk::ComputeCoordinates(const Nz::Vector3f& position) const
	{
		Nz::Vector3f indices = position;
		indices += Nz::Vector3f(m_size) * m_blockSize * 0.5f;
		indices /= m_blockSize;

		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_size.x || pos.y >= m_size.y || pos.z >= m_size.z)
			return std::nullopt;

		return pos;
	}

	std::optional<Chunk::HitBlock> FlatChunk::ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const
	{
		Nz::Vector3f blockPos = hitPos - hitNormal * m_blockSize * 0.25f;
		std::optional<Nz::Vector3ui> blockIndices = ComputeCoordinates(blockPos);
		if (!blockIndices)
			return std::nullopt;

		return HitBlock{
			.direction = DirectionFromNormal(hitNormal),
			.blockIndices = *blockIndices
		};
	}

	void FlatChunk::BuildCollider(const Nz::Vector3ui& dims, Nz::Bitset<Nz::UInt64> collisionCellMask, Nz::FunctionRef<void(const Nz::Boxf& box)> callback)
	{
		auto GetBlockLocalIndex = [&](const Nz::Vector3ui& indices)
		{
			return dims.x * (dims.y * indices.z + indices.y) + indices.x;
		};

		Nz::Vector3f halfDims = Nz::Vector3f(dims) * 0.5f;

		std::optional<Nz::Vector3ui> startPos;
		auto CommitCollider = [&](unsigned int endX)
		{
			if (!startPos)
				return;

			unsigned int endY = startPos->y;
			unsigned int endZ = startPos->z;

			// Try to grow on Y
			for (endY += 1; endY < dims.y; ++endY)
			{
				auto CheckX = [&]
				{
					for (unsigned int x = startPos->x; x <= endX; ++x)
					{
						if (!collisionCellMask[GetBlockLocalIndex({ x, endY, endZ })])
							return false;
					}

					return true;
				};

				if (!CheckX())
					break;
			}
			endY--;

			// Try to grow on Z
			for (endZ += 1; endZ < dims.z; ++endZ)
			{
				auto CheckXY = [&]
				{
					for (unsigned int y = startPos->y; y <= endY; ++y)
					{
						for (unsigned int x = startPos->x; x <= endX; ++x)
						{
							if (!collisionCellMask[GetBlockLocalIndex({ x, y, endZ })])
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
						collisionCellMask[GetBlockLocalIndex({ x, y, z })] = false;
				}
			}

			Nz::Vector3f startOffset(startPos->x, startPos->z, startPos->y);
			Nz::Vector3f endOffset(endX + 1, endZ + 1, endY + 1);

			Nz::Vector3f pos = startOffset - halfDims;
			Nz::Vector3f size = endOffset - startOffset;

			callback(Nz::Boxf(pos, size));

			startPos.reset();
		};

		for (unsigned int z = 0; z < dims.z; ++z)
		{
			for (unsigned int y = 0; y < dims.y; ++y)
			{
				for (unsigned int x = 0; x < dims.x; ++x)
				{
					if (!collisionCellMask[GetBlockLocalIndex({ x, y, z })])
					{
						CommitCollider(x - 1);
						continue;
					}

					if (!startPos)
						startPos = { x, y, z };
				}
				CommitCollider(dims.x - 1);
			}
		}
	}
}
