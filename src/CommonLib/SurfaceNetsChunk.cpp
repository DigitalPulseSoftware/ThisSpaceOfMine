// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/Direction.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <map>
#include <numeric>
#include <spdlog/spdlog.h>

// Thanks a lot to:
// https://github.com/Q-Minh/naive-surface-nets
// https://github.com/MrAlvaroRamirez/Unity-SurfaceNet-VoxelTerrain
// https://github.com/bigos91/fastMarchingCubes

namespace tsom
{
	namespace
	{
		std::atomic_int64_t s_chunkColliderBuildCount = 0;
		std::atomic_int64_t s_chunkColliderBuildTime = 0;
		std::atomic_int64_t s_chunkMeshBuildCount = 0;
		std::atomic_int64_t s_chunkMeshBuildTime = 0;

		constexpr std::array s_neighborChunkOffset = {
			ChunkIndices(-1, -1, -1),
			ChunkIndices(-1, -1,  0),
			ChunkIndices(-1, -1,  1),
			ChunkIndices(-1,  0, -1),
			ChunkIndices(-1,  0,  0),
			ChunkIndices(-1,  0,  1),
			ChunkIndices(-1,  1, -1),
			ChunkIndices(-1,  1,  0),
			ChunkIndices(-1,  1,  1),

			ChunkIndices( 0, -1, -1),
			ChunkIndices( 0, -1,  0),
			ChunkIndices( 0, -1,  1),
			ChunkIndices( 0,  0, -1),
			// 0,0,0 not included
			ChunkIndices( 0,  0,  1),
			ChunkIndices( 0,  1, -1),
			ChunkIndices( 0,  1,  0),
			ChunkIndices( 0,  1,  1),

			ChunkIndices( 1, -1, -1),
			ChunkIndices( 1, -1,  0),
			ChunkIndices( 1, -1,  1),
			ChunkIndices( 1,  0, -1),
			ChunkIndices( 1,  0,  0),
			ChunkIndices( 1,  0,  1),
			ChunkIndices( 1,  1, -1),
			ChunkIndices( 1,  1,  0),
			ChunkIndices( 1,  1,  1),
		};
		
		constexpr Nz::Vector3i s_axis[3] = {
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
		};

		constexpr Nz::Vector3i s_voxelQuads[6][4] = {
			{
				Nz::Vector3i(-1, -1, -1),
				Nz::Vector3i(-1, 0, -1),
				Nz::Vector3i(-1, 0, 0),
				Nz::Vector3i(-1, -1, 0)
			},
			{
				Nz::Vector3i(0, 0, -1),
				Nz::Vector3i(0, 0, 0),
				Nz::Vector3i(-1, 0, 0),
				Nz::Vector3i(-1, 0, -1)
			},
			{
				Nz::Vector3i(0, 0, 0),
				Nz::Vector3i(0, -1, 0),
				Nz::Vector3i(-1, -1, 0),
				Nz::Vector3i(-1, 0, 0)
			},
			{
				Nz::Vector3i(0, 0, -1),
				Nz::Vector3i(0, -1, -1),
				Nz::Vector3i(0, -1, 0),
				Nz::Vector3i(0, 0, 0)
			},
			{
				Nz::Vector3i(-1, -1, -1),
				Nz::Vector3i(-1, -1, 0),
				Nz::Vector3i(0, -1, 0),
				Nz::Vector3i(0, -1, -1)
			},
			{
				Nz::Vector3i(0, -1, -1),
				Nz::Vector3i(0, 0, -1),
				Nz::Vector3i(-1, 0, -1),
				Nz::Vector3i(-1, -1, -1)
			}
		};

		constexpr Nz::Vector3i s_edgeOffsets[12][2] = {
			// Edges on min Z axis
			{ { 0, 0, 0 }, { 1, 0, 0 } },
			{ { 1, 0, 0 }, { 1, 1, 0 } },
			{ { 1, 1, 0 }, { 0, 1, 0 } },
			{ { 0, 1, 0 }, { 0, 0, 0 } },
			// Edges on max Z axis
			{ { 0, 0, 1 }, { 1, 0, 1 } },
			{ { 1, 0, 1 }, { 1, 1, 1 } },
			{ { 1, 1, 1 }, { 0, 1, 1 } },
			{ { 0, 1, 1 }, { 0, 0, 1 } },
			// Edges connecting min Z to max Z
			{ { 0, 0, 0 }, { 0, 0, 1 } },
			{ { 1, 0, 0 }, { 1, 0, 1 } },
			{ { 1, 1, 0 }, { 1, 1, 1 } },
			{ { 0, 1, 0 }, { 0, 1, 1 } },
		};

		constexpr Nz::Vector3ui s_edges[] = {
			{ 0, 0, 0 },
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 },
			{ 1, 1, 0 },
			{ 1, 0, 1 },
			{ 0, 1, 1 },
			{ 1, 1, 1 }
		};

		constexpr Nz::Vector3f s_edgeOffset[] = {
			{ 0, 0, 0 }, // 0
			{ 1, 0, 0 }, // 1
			{ 0, 1, 0 }, // 2
			{ 0, 0, 1 }, // 3
			{ 1, 1, 0 }, // 4
			{ 1, 0, 1 }, // 5
			{ 0, 1, 1 }, // 6
			{ 1, 1, 1 }  // 7
		};

		constexpr std::array<Nz::BoxCorner, 4> s_topCorners = { Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightTopNear, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::LeftBottomNear };
		constexpr std::array<Nz::BoxCorner, 4> s_bottomCorners = { Nz::BoxCorner::RightTopFar, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::RightBottomFar };

		constexpr Nz::EnumArray<Direction, unsigned int> s_dirToAxis = { 1, 5, 4, 0, 3, 2 };
	}

	std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> SurfaceNetsChunk::BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale) const
	{
		Nz::Vector3f offset = (Nz::Vector3f(blockIndices.x, blockIndices.z, blockIndices.y) - Nz::Vector3f(m_size) * 0.5f + Nz::Vector3f(0.5f)) * m_blockSize;
		return { std::make_shared<Nz::BoxCollider3D>(Nz::Vector3f(m_blockSize * scale)), offset };
	}

	std::shared_ptr<Nz::Collider3D> SurfaceNetsChunk::BuildCollider(std::size_t layerIndex) const
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::Vector3f> positions;
		std::vector<Nz::UInt32> triangleUserdata;

		auto AddVertices = [&](const Nz::Vector3ui& blockIndices, Direction direction)
		{
			VertexAttributes vertexAttributes;

			vertexAttributes.firstIndex = Nz::SafeCast<Nz::UInt32>(positions.size());
			positions.resize(positions.size() + 4);
			vertexAttributes.position = Nz::SparsePtr<Nz::Vector3f>(&positions[vertexAttributes.firstIndex]);

			Nz::UInt32 localBlockIndex = GetBlockLocalIndex(blockIndices) * 6 + static_cast<Nz::UInt32>(direction);
			triangleUserdata.push_back(localBlockIndex);
			triangleUserdata.push_back(localBlockIndex);

			return vertexAttributes;
		};

		BuildMesh(layerIndex, indices, AddVertices, false);
		if (indices.empty())
			return nullptr;

		Nz::MeshCollider3D::Settings meshSettings;
		meshSettings.indexCount = indices.size();
		meshSettings.indices = indices.data();
		meshSettings.vertexCount = positions.size();
		meshSettings.vertices = &positions[0];
		meshSettings.triangleUserdata = &triangleUserdata[0];

		return std::make_shared<Nz::MeshCollider3D>(meshSettings);
	}

	void SurfaceNetsChunk::BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace) const
	{
		BuildMesh(layerIndex, indices, addFace, true);
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> SurfaceNetsChunk::ComputeBlockCorners(const Nz::Vector3ui& indices) const
	{
		NeighborChunkArray neighborChunks;

		// Find every neighbor chunk required, based on block position
		std::array<int, 2> xs{ 0, 0 };
		std::array<int, 2> ys{ 0, 0 };
		std::array<int, 2> zs{ 0, 0 };

		std::size_t nx = 1, ny = 1, nz = 1;

		if (indices.x == 0)
			xs[nx++] = -1;
		else if (indices.x == m_size.x - 1)
			xs[nx++] = 1;

		if (indices.y == 0)
			zs[nz++] = -1;
		else if (indices.y == m_size.y - 1)
			zs[nz++] = 1;

		if (indices.z == 0)
			ys[ny++] = -1;
		else if (indices.z == m_size.z - 1)
			ys[ny++] = 1;

		MultiChunkReadLock chunkLock;
		for (std::size_t ix = 0; ix < nx; ++ix)
		{
			for (std::size_t iy = 0; iy < ny; ++iy)
			{
				for (std::size_t iz = 0; iz < nz; ++iz)
				{
					int dx = xs[ix];
					int dy = ys[iy];
					int dz = zs[iz];

					if (dx == 0 && dy == 0 && dz == 0)
						continue;

					ChunkIndices dir(dx, dy, dz);
					const Chunk* chunk = m_owner.GetChunk(m_indices + dir);
					neighborChunks[GetNeighborIndex(ChunkIndices(dx, dy, dz))] = chunk;

					if (chunk)
						chunkLock.AddChunk(chunk);
				}
			}
		}

		chunkLock.Lock();
		return BuildCorners(indices, neighborChunks);
	}

	std::optional<Nz::Vector3ui> SurfaceNetsChunk::ComputeCoordinates(const Nz::Vector3f& position) const
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

	std::optional<Chunk::HitBlock> SurfaceNetsChunk::ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const
	{
		std::uint32_t remainder;
		const Nz::Collider3D* subCollider = collider.GetSubCollider(hitSubshapeId, remainder);
		if (!subCollider)
			return std::nullopt;

		Nz::UInt32 userdata = SafeCast<const Nz::MeshCollider3D*>(subCollider)->GetTriangleUserData(remainder);

		return HitBlock{
			.direction = static_cast<Direction>(userdata % 6),
			.blockIndices = GetBlockLocalIndices(userdata / 6)
		};
	}

	Nz::Int64 SurfaceNetsChunk::GetColliderBuildTime()
	{
		std::int64_t buildCount = std::max(s_chunkColliderBuildCount.load(), std::int64_t(1));
		return s_chunkColliderBuildTime / buildCount;
	}

	Nz::Int64 SurfaceNetsChunk::GetMeshBuildTime()
	{
		std::int64_t buildCount = std::max(s_chunkMeshBuildCount.load(), std::int64_t(1));
		return s_chunkMeshBuildTime / buildCount;
	}

	void SurfaceNetsChunk::ResetTime()
	{
		s_chunkColliderBuildCount = 0;
		s_chunkColliderBuildTime = 0;
		s_chunkMeshBuildCount = 0;
		s_chunkMeshBuildTime = 0;
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> SurfaceNetsChunk::BuildCorners(const Nz::Vector3ui& indices, const NeighborChunkArray& neighborChunks) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize + Nz::Vector3f(m_blockSize);
		Nz::Vector3f blockOffset(blockPos.x, blockPos.z, blockPos.y);

		BlockIndex blockIndex = GetBlockContent(indices);
		const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		auto corners = box.GetCorners();

		for (Direction direction : { Direction::Up, Direction::Down })
		{
			unsigned int axis = s_dirToAxis[direction];
			const std::array<Nz::BoxCorner, 4>& boxCorners = (direction == Direction::Up) ? s_topCorners : s_bottomCorners;

			for (std::size_t vertIndex = 0; vertIndex < 4; ++vertIndex)
			{
				Nz::Vector3f total = Nz::Vector3f::Zero();
				unsigned int count = 0;

				bool isSmooth = blockData.isSmooth;
				if (isSmooth)
				{
					for (int z = 0; z < 12; ++z)
					{
						Nz::Vector3i edgePos = s_voxelQuads[axis][vertIndex] + s_edgeOffsets[z][0];
						Nz::Vector3i edgeNeighborPos = s_voxelQuads[axis][vertIndex] + s_edgeOffsets[z][1];

						BlockIndex edge1 = GetNeighborBlock(neighborChunks, indices, edgePos);
						BlockIndex edge2 = GetNeighborBlock(neighborChunks, indices, edgeNeighborPos);

						const auto& edge1BlockData = m_blockLibrary.GetBlockData(edge1);
						const auto& edge2BlockData = m_blockLibrary.GetBlockData(edge2);

						if (!edge2BlockData.isSmooth)
						{
							isSmooth = false;
							break;
						}

						bool isEdge1Visible = !edge1BlockData.isTransparent;
						bool isEdge2Visible = !edge2BlockData.isTransparent;

						if (isEdge1Visible != isEdge2Visible)
						{
							Nz::Vector3f edge1Pos(edgePos.x, edgePos.z, edgePos.y);
							Nz::Vector3f edge2Pos(edgeNeighborPos.x, edgeNeighborPos.z, edgeNeighborPos.y);

							float edge1Density = edge1BlockData.density;
							float edge2Density = edge2BlockData.density;

							Nz::Vector3f midPoint;
							if (edge1BlockData.isSmooth != edge2BlockData.isSmooth)
								midPoint = edge1Pos; //< edge2BlockData.isSmooth cannot be false here
							else
								midPoint = (edge1Pos * edge1Density + edge2Pos * edge2Density) / (edge1Density + edge2Density);

							total += midPoint;
							count++;
						}
					}
				}

				if (count == 0)
					isSmooth = false;

				if (isSmooth)
					corners[boxCorners[vertIndex]] = blockOffset + m_blockSize * total / float(count) - Nz::Vector3f(m_blockSize * 0.5f);
				else
				{
					Nz::Vector3f offset(s_voxelQuads[axis][vertIndex]);
					offset = { offset.x, offset.z, offset.y };

					corners[boxCorners[vertIndex]] = blockOffset + m_blockSize * offset;
				}
			}
		}

		return corners;
	}

	void SurfaceNetsChunk::BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace, bool generateVisualMesh) const
	{
		// Find and lock all neighbor chunks to avoid discrepancies between chunks
		NeighborChunkArray neighborChunks;

		MultiChunkReadLock chunkLock;
		for (const ChunkIndices& dir : s_neighborChunkOffset)
		{
			const Chunk* chunk = m_owner.GetChunk(m_indices + dir);

			neighborChunks[GetNeighborIndex(dir)] = chunk;
			if (chunk)
				chunkLock.AddChunk(chunk);
		}

		chunkLock.Lock();

		Nz::HighPrecisionClock clock;

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					Nz::Vector3ui blockIndices(x, y, z);

					BlockIndex blockContent = GetBlockContent(blockIndices);
					if (blockContent == EmptyBlockIndex)
						continue;

					const auto& blockData = m_blockLibrary.GetBlockData(blockContent);
					if (blockData.layerIndex != layerIndex)
						continue;

					std::optional<Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f>> corners;

					auto DrawFace = [&](Direction direction, bool interiorFace, const std::array<Nz::BoxCorner, 4>& faceCorners)
					{
						if (!corners)
							corners = BuildCorners(blockIndices, neighborChunks);

						VertexAttributes vertexAttributes = addFace(blockIndices, direction);
						assert(vertexAttributes.position);

						constexpr std::array<Nz::UInt32, 6> s_indices = { 0, 2, 1, 1, 2, 3 };
						constexpr std::array<Nz::UInt32, 6> s_reversedIndices = { 0, 1, 2, 1, 3, 2 };

						const std::array<Nz::UInt32, 6>& faceIndices = (interiorFace) ? s_reversedIndices : s_indices;

						for (Nz::UInt32 index : faceIndices)
							indices.push_back(vertexAttributes.firstIndex + index);

						for (std::size_t vertIndex = 0; vertIndex < 4; ++vertIndex)
							vertexAttributes.position[vertIndex] = (*corners)[faceCorners[vertIndex]];

						if (vertexAttributes.normal)
						{
							// Per face normal
							Nz::Vector3f n0 = Nz::Vector3f::CrossProduct(vertexAttributes.position[faceIndices[1]] - vertexAttributes.position[faceIndices[0]], vertexAttributes.position[faceIndices[2]] - vertexAttributes.position[faceIndices[0]]);
							Nz::Vector3f n1 = Nz::Vector3f::CrossProduct(vertexAttributes.position[faceIndices[4]] - vertexAttributes.position[faceIndices[3]], vertexAttributes.position[faceIndices[5]] - vertexAttributes.position[faceIndices[3]]);

							Nz::Vector3f faceNormal = Nz::Vector3f::Normalize(n0 + n1);

							for (unsigned int i = 0; i < 4; ++i)
								vertexAttributes.normal[i] = faceNormal;
						}

						if (vertexAttributes.tangent)
						{
							Nz::Vector3f faceTangent = Nz::Vector3f::Normalize(vertexAttributes.position[1] - vertexAttributes.position[0]);

							for (std::size_t i = 0; i < 4; ++i)
								vertexAttributes.tangent[i] = faceTangent;
						}

						if (vertexAttributes.uv)
						{
							std::size_t textureIndex = blockData.texIndices[Direction::Up];
							float sliceIndex = textureIndex;
							for (std::size_t i = 0; i < 4; ++i)
								vertexAttributes.uv[i] = { 0.f, 0.f, sliceIndex };
						}
					};

					auto IsTransparent = [&](BlockIndex neighborBlockIndex)
					{
						// don't render faces between blocks of the same type even if transparent
						if (blockContent == neighborBlockIndex)
							return false;

						const auto& neighborBlockData = m_blockLibrary.GetBlockData(neighborBlockIndex);
						return neighborBlockData.isTransparent;
					};

					// Up
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Up])))
					{
						DrawFace(Direction::Up, false, { Nz::BoxCorner::RightTopNear, Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::LeftBottomNear });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Up, true, { Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightTopNear, Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::RightBottomNear });
					}

					// Down
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Down])))
					{
						DrawFace(Direction::Down, false, { Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::RightTopFar, Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::RightBottomFar });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Down, true, { Nz::BoxCorner::RightTopFar, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::LeftBottomFar });
					}

					// Front
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Front])))
					{
						DrawFace(Direction::Front, false, { Nz::BoxCorner::RightTopFar, Nz::BoxCorner::RightTopNear, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::RightBottomNear });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Front, true, { Nz::BoxCorner::RightTopNear, Nz::BoxCorner::RightTopFar, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::RightBottomFar });
					}

					// Back
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Back])))
					{
						DrawFace(Direction::Back, false, { Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::LeftBottomFar });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Back, true, { Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::LeftBottomNear });
					}

					// Left
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Left])))
					{
						DrawFace(Direction::Left, false, { Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::LeftBottomFar });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Left, true, { Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::RightBottomFar });
					}

					// Right
					if (IsTransparent(GetNeighborBlock(neighborChunks, blockIndices, s_blockDirOffset[Direction::Right])))
					{
						DrawFace(Direction::Right, false, { Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightTopNear, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::RightTopFar });
						if (blockData.isDoubleSided && generateVisualMesh)
							DrawFace(Direction::Right, true, { Nz::BoxCorner::RightTopNear, Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightTopFar, Nz::BoxCorner::LeftTopFar });
					}
				}
			}
		}

		if (generateVisualMesh)
		{
			s_chunkMeshBuildTime += clock.GetElapsedTime().AsNanoseconds();
			s_chunkMeshBuildCount++;
		}
		else
		{
			s_chunkColliderBuildTime += clock.GetElapsedTime().AsNanoseconds();
			s_chunkColliderBuildCount++;
		}
	}

	BlockIndex SurfaceNetsChunk::GetNeighborBlock(const NeighborChunkArray& neighborChunks, Nz::Vector3ui indices, const Nz::Vector3i& offset) const
	{
		ChunkIndices chunkIndices = m_indices;
		std::swap(chunkIndices.y, chunkIndices.z);
		bool crossedBoundaries = false;

		for (unsigned int axis : { 0, 1, 2 })
		{
			int axisOffset = offset[axis];
			assert(axisOffset >= -1 && axisOffset <= 1);

			int index = static_cast<int>(indices[axis]);
			index += axisOffset;

			int size = int(m_size[axis]);

			if (index < 0)
			{
				index += size;
				chunkIndices[axis]--;
				crossedBoundaries = true;
			}
			else if (index >= size)
			{
				index -= size;
				chunkIndices[axis]++;
				crossedBoundaries = true;
			}

			assert(index >= 0 && index < size);
			indices[axis] = static_cast<unsigned int>(index);
		}

		std::swap(chunkIndices.y, chunkIndices.z);

		if (crossedBoundaries)
		{
			const Chunk* chunk = neighborChunks[GetNeighborIndex(chunkIndices - m_indices)];
			if (!chunk)
				return EmptyBlockIndex;

			if (!chunk->HasContent())
				return EmptyBlockIndex;

			return chunk->GetBlockContent(indices);
		}
		else
			return GetBlockContent(indices);
	};
}
