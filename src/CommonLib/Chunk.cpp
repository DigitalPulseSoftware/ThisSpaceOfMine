// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Chunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Core/VertexStruct.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <cassert>
#include <numeric>

namespace tsom
{
	Chunk::~Chunk() = default;

	void Chunk::BuildMesh(const BlockLibrary& blockManager, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(Nz::UInt32)>& addVertices) const
	{
		auto DrawFace = [&](BlockIndex blockIndex, const Nz::Vector3f& blockCenter, const std::array<Nz::Vector3f, 4>& pos)
		{
			VertexAttributes vertexAttributes = addVertices(pos.size());
			assert(vertexAttributes.position);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			Nz::Vector3f faceCenter = std::accumulate(pos.begin(), pos.end(), Nz::Vector3f::Zero()) / pos.size();
			Nz::Vector3f faceDirection = Nz::Vector3f::Normalize(faceCenter - blockCenter);

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = faceDirection;
			}

			if (vertexAttributes.uv)
			{
				// Get face up vector
				Nz::Vector3f faceUp = s_dirNormals[DirectionFromNormal(Nz::Vector3f::Normalize(faceCenter - gravityCenter))];

				// Make up the rotation from the face up to the regular up
				Nz::Quaternionf upRotation = Nz::Quaternionf::RotationBetween(faceUp, Nz::Vector3f::Up());

				// Compute texture direction based on face direction in regular orientation
				Direction texDirection = DirectionFromNormal(upRotation * faceDirection);

				const auto& blockData = blockManager.GetBlockData(blockIndex);
				std::size_t textureIndex = blockData.texIndices[texDirection];

				// Compute UV
				float sliceIndex = textureIndex;
				for (std::size_t i = 0; i < pos.size(); ++i)
				{
					// Get vector from center to corner (no need to normalize) and use it to compute UV
					// This is similar to the way a GPU compute UV when sampling a cubemap: https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/5337472/
					Nz::Vector3f dir = upRotation * (pos[i] - blockCenter);
					Nz::Vector3f dirAbs = dir.GetAbs();

					float mag = 0.f;
					Nz::Vector2f uv;
					switch (texDirection) //< TODO: texture direction should be defined by dir to handle corners
					{
						case Direction::Back:
						case Direction::Front:
						{
							mag = 0.5f / dirAbs.x;
							uv = { dir.x < 0.f ? -dir.z : dir.z, -dir.y };
							break;
						}

						case Direction::Down:
						case Direction::Up:
						{
							mag = 0.5f / dirAbs.y;
							uv = { dir.x, dir.y < 0.f ? -dir.z : dir.z };
							break;
						}

						case Direction::Left:
						case Direction::Right:
						{
							mag = 0.5f / dirAbs.z;
							uv = { dir.z < 0.f ? dir.x : -dir.x, -dir.y };
							break;
						}
					}

					vertexAttributes.uv[i] = Nz::Vector3f(uv * mag + Nz::Vector2f(0.5f), sliceIndex);
				}
			}

			indices.push_back(vertexAttributes.firstIndex);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 1);

			indices.push_back(vertexAttributes.firstIndex + 1);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 3);
		};

		// Find and lock all neighbor chunks to avoid discrepancies between chunks
		Nz::EnumArray<Direction, const Chunk*> neighborChunks;
		for (auto&& [dir, chunk] : neighborChunks.iter_kv())
		{
			chunk = m_owner.GetChunk(m_indices + s_dirOffset[dir]);
			if (!chunk)
				continue;

			chunk->LockRead();
		}

		NAZARA_DEFER(
		{
			for (const Chunk* chunk : neighborChunks)
			{
				if (chunk)
					chunk->UnlockRead();
			}
		});

		auto GetNeighborBlock = [&](Nz::Vector3ui indices, const Nz::Vector3i& offsets) -> std::optional<BlockIndex>
		{
			ChunkIndices chunkIndices = m_indices;
			std::swap(chunkIndices.y, chunkIndices.z);

			for (unsigned int axis : { 0, 1, 2 })
			{
				unsigned int& index = indices[axis];
				int offset = offsets[axis];
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
				ChunkIndices neighborIndices = chunkIndices - m_indices;

				// FIXME
				for (auto&& [dir, dirIndices] : s_dirOffset.iter_kv())
				{
					if (neighborIndices == dirIndices)
					{
						const Chunk* chunk = neighborChunks[dir];
						if (!chunk)
							return {};

						if (!chunk->HasContent())
							return {};

						return chunk->GetBlockContent(indices);
					}
				}

				NazaraErrorFmt("unexpected neighbor indices {};{};{}", neighborIndices.x, neighborIndices.y, neighborIndices.z);
			}
			else
				return GetBlockContent(indices);
		};

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					BlockIndex blockIndex = GetBlockContent({ x, y, z });
					if (blockIndex == EmptyBlockIndex)
						continue;

					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners({ x, y, z });

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					// Up
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, 1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear] });

					// Down
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, -1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });

					// Front
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, -1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::RightBottomNear] });

					// Back
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::LeftBottomFar] });

					// Left
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { -1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });

					// Right
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar] });
				}
			}
		}
	}

	void Chunk::Serialize(const BlockLibrary& blockLibrary, Nz::ByteStream& byteStream)
	{
		byteStream << Constants::ChunkBinaryVersion;
		byteStream << m_size;

		std::vector<BlockIndex> serializationIndices(m_blockTypeCount.size());
		Nz::UInt16 nextUniqueIndex = 0;

		for (BlockIndex i = 0; i < m_blockTypeCount.size(); ++i)
		{
			if (m_blockTypeCount[i] == 0)
				continue;

			serializationIndices[i] = nextUniqueIndex++;
		}

		byteStream << Nz::SafeCast<Nz::UInt16>(nextUniqueIndex);
		for (BlockIndex i = 0; i < m_blockTypeCount.size(); ++i)
		{
			if (m_blockTypeCount[i] == 0)
				continue;

			byteStream << blockLibrary.GetBlockData(i).name;
		}

		// nextUniqueIndex is the number of bits required to store all the different block types used
		if (nextUniqueIndex > 8)
		{
			for (BlockIndex blockIndex : m_blocks)
				byteStream << static_cast<Nz::UInt16>(serializationIndices[blockIndex]);
		}
		else
		{
			for (BlockIndex blockIndex : m_blocks)
				byteStream << static_cast<Nz::UInt8>(serializationIndices[blockIndex]);
		}
	}

	void Chunk::Deserialize(const BlockLibrary& blockLibrary, Nz::ByteStream& byteStream)
	{
		Nz::UInt32 chunkBinaryVersion;
		byteStream >> chunkBinaryVersion;

		if (chunkBinaryVersion != Constants::ChunkBinaryVersion)
			throw std::runtime_error("incompatible chunk version");

		Nz::Vector3ui chunkSize;
		byteStream >> chunkSize;

		if (chunkSize != m_size)
			throw std::runtime_error("incompatible chunk size");

		std::vector<BlockIndex> deserializationIndices;

		Nz::UInt16 blockTypeCount;
		byteStream >> blockTypeCount;

		deserializationIndices.reserve(blockTypeCount);

		std::string blockName;
		for (Nz::UInt16 i = 0; i < blockTypeCount; ++i)
		{
			byteStream >> blockName;

			BlockIndex blockIndex = blockLibrary.GetBlockIndex(blockName);
			if (blockIndex == InvalidBlockIndex)
				throw std::runtime_error("unknown block " + blockName);

			deserializationIndices.push_back(blockIndex);
		}

		std::vector<BlockIndex> blocks(m_blocks.size());
		if (blockTypeCount > 8)
		{
			for (BlockIndex& blockIndex : blocks)
			{
				Nz::UInt16 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}
		else
		{
			for (BlockIndex& blockIndex : blocks)
			{
				Nz::UInt8 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}

		m_blocks = std::move(blocks);
		OnChunkReset();
	}

	void Chunk::OnChunkReset()
	{
		std::fill(m_blockTypeCount.begin(), m_blockTypeCount.end(), 0);
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			BlockIndex blockContent = m_blocks[blockIndex];
			m_collisionCellMask[blockIndex] = (blockContent != EmptyBlockIndex);

			if (blockContent >= m_blockTypeCount.size())
				m_blockTypeCount.resize(blockContent + 1);

			m_blockTypeCount[blockContent]++;
		}

		OnReset(this);
	}
}
