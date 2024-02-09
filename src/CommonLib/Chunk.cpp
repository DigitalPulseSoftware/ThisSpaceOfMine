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

	void Chunk::BuildMesh(std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(Nz::UInt32)>& addVertices) const
	{
		auto DrawFace = [&](BlockIndex blockIndex, const Nz::Vector3f& blockCenter, const std::array<Nz::Vector3f, 4>& pos)
		{
			VertexAttributes vertexAttributes = addVertices(pos.size());
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

			if (vertexAttributes.tangent)
			{
				Nz::Vector3f edgeCenter = (pos[0] + pos[1]) * 0.5f;
				Nz::Vector3f tangent = Nz::Vector3f::Normalize(edgeCenter - faceCenter);

				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.tangent[i] = tangent;
			}

			if (vertexAttributes.uv)
			{
				Nz::Vector3f faceUp = s_dirNormals[DirectionFromNormal(Nz::Vector3f::Normalize(faceCenter - gravityCenter))];

				// Make up the rotation from the face up to the regular up
				Nz::Quaternionf upRotation = Nz::Quaternionf::RotationBetween(faceUp, Nz::Vector3f::Up());

				// Compute texture direction based on face direction in regular orientation
				Direction texDirection = DirectionFromNormal(upRotation * faceDirection);

				const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);
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

			// deform positions after generating UV
			if (DeformPositions(vertexAttributes.position, pos.size()))
			{
				if (vertexAttributes.normal && vertexAttributes.tangent)
					DeformNormalsAndTangents(vertexAttributes.normal, vertexAttributes.tangent, faceDirection, vertexAttributes.position, pos.size());
				else if (vertexAttributes.normal)
					DeformNormals(vertexAttributes.normal, faceDirection, vertexAttributes.position, pos.size());
			}
		};

		// Find and lock all neighbor chunks to avoid discrepancies between chunks
		Nz::EnumArray<Direction, const Chunk*> neighborChunks;
		for (auto&& [dir, chunk] : neighborChunks.iter_kv())
		{
			chunk = m_owner.GetChunk(m_indices + s_chunkDirOffset[dir]);
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

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					BlockIndex blockIndex = GetBlockContent({ x, y, z });
					if (blockIndex == EmptyBlockIndex)
						continue;

					const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

					// Get unaltered voxel corners and deform them next
					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = Chunk::ComputeVoxelCorners({ x, y, z });

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					auto IsTransparent = [&](BlockIndex neighborBlockIndex)
					{
						// don't render faces between blocks of the same type even if transparent
						if (blockIndex == neighborBlockIndex)
							return false;

						const auto& neighborBlockData = m_blockLibrary.GetBlockData(neighborBlockIndex);
						return neighborBlockData.isTransparent;
					};

					// Up
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Up); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear] });
					}

					// Down
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Down); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
					}

					// Front
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Front); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::RightBottomNear] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Back
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Back); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::LeftBottomNear] });
					}

					// Left
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Left); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomFar], corners[Nz::BoxCorner::LeftBottomFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftBottomNear], corners[Nz::BoxCorner::RightBottomNear], corners[Nz::BoxCorner::LeftBottomFar], corners[Nz::BoxCorner::RightBottomFar] });
					}

					// Right
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, Direction::Right); !neighborOpt || IsTransparent(*neighborOpt))
					{
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopFar], corners[Nz::BoxCorner::RightTopFar] });
						if (blockData.isDoubleSided)
							DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::RightTopNear], corners[Nz::BoxCorner::LeftTopNear], corners[Nz::BoxCorner::RightTopFar], corners[Nz::BoxCorner::LeftTopFar] });
					}
				}
			}
		}
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> Chunk::ComputeVoxelCorners(const Nz::Vector3ui& indices) const
	{
		float fX = indices.x * m_blockSize;
		float fY = indices.y * m_blockSize;
		float fZ = indices.z * m_blockSize;

		Nz::Boxf box(fX, fZ, fY, m_blockSize, m_blockSize, m_blockSize);
		return box.GetCorners();
	}

	void Chunk::DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		/* nothing to do */
	}

	void Chunk::DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const
	{
		/* nothing to do */
	}

	bool Chunk::DeformPositions(Nz::SparsePtr<Nz::Vector3f> /*positions*/, std::size_t /*positionCount*/) const
	{
		/* nothing to do */
		return false;
	}

	void Chunk::Deserialize(Nz::ByteStream& byteStream)
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

			BlockIndex blockIndex = m_blockLibrary.GetBlockIndex(blockName);
			if (blockIndex == InvalidBlockIndex)
				throw std::runtime_error("unknown block " + blockName);

			deserializationIndices.push_back(blockIndex);
		}

		Reset();
		if (blockTypeCount > 8)
		{
			for (BlockIndex& blockIndex : m_blocks)
			{
				Nz::UInt16 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}
		else
		{
			for (BlockIndex& blockIndex : m_blocks)
			{
				Nz::UInt8 value;
				byteStream >> value;

				blockIndex = deserializationIndices[value];
			}
		}

		OnChunkReset();
	}

	void Chunk::Serialize(Nz::ByteStream& byteStream) const
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

			byteStream << m_blockLibrary.GetBlockData(i).name;
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

	void Chunk::UpdateBlock(const Nz::Vector3ui& indices, BlockIndex newBlock)
	{
		NazaraAssert(!m_blocks.empty(), "chunk has not been reset");

		const auto& blockData = m_blockLibrary.GetBlockData(newBlock);

		unsigned int blockIndex = GetBlockLocalIndex(indices);
		BlockIndex oldContent = m_blocks[blockIndex];
		m_blocks[blockIndex] = newBlock;
		m_collisionCellMask[blockIndex] = blockData.hasCollisions;

		m_blockTypeCount[oldContent]--;
		if (newBlock >= m_blockTypeCount.size())
			m_blockTypeCount.resize(newBlock + 1);

		m_blockTypeCount[newBlock]++;

		OnBlockUpdated(this, indices, newBlock);
	}

	void Chunk::OnChunkReset()
	{
		std::fill(m_blockTypeCount.begin(), m_blockTypeCount.end(), 0);
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			BlockIndex blockContent = m_blocks[blockIndex];
			const auto& blockData = m_blockLibrary.GetBlockData(blockContent);
			m_collisionCellMask[blockIndex] = blockData.hasCollisions;

			if (blockContent >= m_blockTypeCount.size())
				m_blockTypeCount.resize(blockContent + 1);

			m_blockTypeCount[blockContent]++;
		}

		OnReset(this);
	}
}
