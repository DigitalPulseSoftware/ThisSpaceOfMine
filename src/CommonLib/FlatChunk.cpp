// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <numeric>

namespace tsom
{
	std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> FlatChunk::BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale) const
	{
		Nz::Vector3f offset = (Nz::Vector3f(blockIndices.x, blockIndices.z, blockIndices.y) - Nz::Vector3f(m_size) * 0.5f + Nz::Vector3f(0.5f)) * m_blockSize;
		return { std::make_shared<Nz::BoxCollider3D>(Nz::Vector3f(m_blockSize * scale)), offset };
	}

	auto FlatChunk::BuildCollider(std::size_t layerIndex, bool /*buildDynamicCollider*/) const -> ColliderData
	{
		std::vector<Nz::CompoundCollider3D::ChildCollider> childColliders;

		auto AddBox = [&](const Nz::Boxf& box)
		{
			auto& childCollider = childColliders.emplace_back();
			childCollider.offset = box.GetCenter() * m_blockSize;
			childCollider.collider = std::make_shared<Nz::BoxCollider3D>(box.GetLengths() * m_blockSize);
		};

		BuildCollider(m_size, GetCollisionCellMask(layerIndex), AddBox);

		if (childColliders.empty())
			return { nullptr, nullptr };

		auto collider = std::make_shared<Nz::CompoundCollider3D>(std::move(childColliders));
		return { collider, nullptr };
	}
	
	void FlatChunk::BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace) const
	{
		auto DrawFace = [&](BlockIndex blockContent, const Nz::Vector3ui& blockIndices, Direction direction, const Nz::Vector3f& blockCenter, const std::array<Nz::Vector3f, 4>& pos)
		{
			VertexAttributes vertexAttributes = addFace(blockIndices, direction);
			assert(vertexAttributes.position);

			indices.push_back(vertexAttributes.firstIndex);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 1);

			indices.push_back(vertexAttributes.firstIndex + 1);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 3);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			Nz::Vector3f faceCenter = std::accumulate(pos.begin(), pos.end(), Nz::Vector3f::Zero()) / pos.size();
			Nz::Vector3f faceDirection = Nz::Vector3f::Normalize(faceCenter - blockCenter);

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = faceDirection;
			}

			if (vertexAttributes.blockIndex)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.blockIndex[i] = blockContent;
			}
		};

		Nz::EnumArray<Direction, const Chunk*> neighborChunks;
		MultiChunkReadLock chunkLock;
		for (auto&& [dir, chunk] : neighborChunks.iter_kv())
		{
			chunk = m_owner.GetChunk(m_indices + s_chunkDirOffset[dir]);
			if (chunk)
				chunkLock.AddChunk(chunk);
		}

		chunkLock.Lock();

		auto GetNeighborBlock = [&](Nz::Vector3ui indices, Direction direction) -> std::optional<BlockIndex>
		{
			ChunkIndices chunkIndices = m_indices;
			std::swap(chunkIndices.y, chunkIndices.z);

			for (unsigned int axis : { 0, 1, 2 })
			{
				unsigned int& index = indices[axis];
				int offset = s_blockDirOffset[direction][axis];
				assert(offset >= -1 && offset <= 1);

				if (offset > 0)
				{
					index += offset;
					if (index >= m_size[axis])
					{
						index -= m_size[axis];
						chunkIndices[axis]++;
					}
				}
				else if (offset < 0)
				{
					unsigned int posOffset = std::abs(offset);
					if (posOffset > index)
					{
						index += m_size[axis];
						chunkIndices[axis]--;
					}

					index -= posOffset;
				}
			}

			std::swap(chunkIndices.y, chunkIndices.z);

			if (chunkIndices != m_indices)
			{
				const Chunk* chunk = neighborChunks[direction];
				if (!chunk)
					return {};

				if (!chunk->HasContent())
					return {};

				return chunk->GetBlockContent(indices);
			}
			else
				return GetBlockContent(indices);
		};

		const auto& blockLibrary = m_owner.GetBlockLibrary();
		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					Nz::Vector3ui blockIndices(x, y, z);

					BlockIndex blockIndex = GetBlockContent(blockIndices);
					if (blockIndex == EmptyBlockIndex)
						continue;

					const auto& blockData = blockLibrary.GetBlockData(blockIndex);
					if (blockData.layerIndex != layerIndex)
						continue;

					// Get unaltered voxel corners and deform them next
					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = Chunk::ComputeBlockCorners(blockIndices);

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					auto IsTransparent = [&](BlockIndex neighborBlockIndex)
					{
						// don't render faces between blocks of the same type even if transparent
						if (blockIndex == neighborBlockIndex)
							return false;

						const auto& neighborBlockData = blockLibrary.GetBlockData(neighborBlockIndex);
						return neighborBlockData.isTransparent;
					};

					// Up
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Up); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Up, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Up, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear] });
					}

					// Down
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Down); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Down, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Down, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
					}

					// Front
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Front); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Front, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::RightBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Front, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Back
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Back); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Back, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Back, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::LeftBottomNear] });
					}

					// Left
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Left); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Left, blockCenter, { corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Left, blockCenter, { corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Right
					if (auto neighborOpt = GetNeighborBlock(blockIndices, Direction::Right); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockIndices, Direction::Right, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockIndices, Direction::Right, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar] });
					}
				}
			}
		}
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
