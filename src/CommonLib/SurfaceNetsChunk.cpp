// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Direction.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <NazaraUtils/FixedVector.hpp>
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

		constexpr Nz::Vector3i s_voxelQuads[3][4] = {
			{
				Nz::Vector3i(0, 0, -1),
				Nz::Vector3i(0, -1, -1),
				Nz::Vector3i(0, -1, 0),
				Nz::Vector3i(0, 0, 0)
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

		/*for (std::size_t i = 0; i < indices.size(); i += 3)
		{
			std::swap(indices[i + 1], indices[i + 2]);
		}*/

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

	void SurfaceNetsChunk::BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace, bool generateVisualMesh) const
	{
		// Find and lock all neighbor chunks to avoid discrepancies between chunks
		std::unordered_map<ChunkIndices, const Chunk*> neighborChunks = {
			{ ChunkIndices(-1, -1, -1),	nullptr },
			{ ChunkIndices(-1, -1,  0),	nullptr },
			{ ChunkIndices(-1, -1,  1),	nullptr },
			{ ChunkIndices(-1,  0, -1),	nullptr },
			{ ChunkIndices(-1,  0,  0),	nullptr },
			{ ChunkIndices(-1,  0,  1),	nullptr },
			{ ChunkIndices(-1,  1, -1),	nullptr },
			{ ChunkIndices(-1,  1,  0),	nullptr },
			{ ChunkIndices(-1,  1,  1),	nullptr },

			{ ChunkIndices(0, -1, -1), nullptr },
			{ ChunkIndices(0, -1,  0), nullptr },
			{ ChunkIndices(0, -1,  1), nullptr },
			{ ChunkIndices(0,  0, -1), nullptr },

			{ ChunkIndices(0,  0,  1), nullptr },
			{ ChunkIndices(0,  1, -1), nullptr },
			{ ChunkIndices(0,  1,  0), nullptr },
			{ ChunkIndices(0,  1,  1), nullptr },

			{ ChunkIndices(1, -1, -1), nullptr },
			{ ChunkIndices(1, -1,  0), nullptr },
			{ ChunkIndices(1, -1,  1), nullptr },
			{ ChunkIndices(1,  0, -1), nullptr },
			{ ChunkIndices(1,  0,  0), nullptr },
			{ ChunkIndices(1,  0,  1), nullptr },
			{ ChunkIndices(1,  1, -1), nullptr },
			{ ChunkIndices(1,  1,  0), nullptr },
			{ ChunkIndices(1,  1,  1), nullptr },
		};

		Nz::FixedVector<const Chunk*, 26> chunkLocks;
		for (auto&& [dir, chunk] : neighborChunks)
		{
			chunk = m_owner.GetChunk(m_indices + dir);
			if (chunk)
				chunkLocks.push_back(chunk);
		}

		// We need to lock all chunks at once to avoid deadlocks
		std::size_t lastFailureMutex = Nz::MaxValue();
		for (;;)
		{
			bool succeeded = true;
			for (std::size_t i = 0; i < chunkLocks.size(); ++i)
			{
				if (i != lastFailureMutex && !chunkLocks[i]->TryLockRead())
				{
					succeeded = false;
					// Lock failed, unlock everything and try again
					for (std::size_t j = 0; j < i; ++j)
						chunkLocks[j]->UnlockRead();

					// Lock blocked chunk first to pause thread
					chunkLocks[i]->LockRead();
					lastFailureMutex = i;
					break;
				}
			}

			if (succeeded)
				break;
		}

		NAZARA_DEFER(
		{
			for (const Chunk* chunk : chunkLocks)
				chunk->UnlockRead();
		});

		auto GetNeighborBlock = [&](Nz::Vector3ui indices, Nz::Vector3i direction) -> BlockIndex
		{
			ChunkIndices chunkIndices = m_indices;
			std::swap(chunkIndices.y, chunkIndices.z);

			for (unsigned int axis : { 0, 1, 2 })
			{
				unsigned int& index = indices[axis];
				int offset = direction[axis];
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
				auto it = neighborChunks.find(chunkIndices - m_indices);
				NazaraAssert(it != neighborChunks.end());

				const Chunk* chunk = it->second;
				if (!chunk)
					return EmptyBlockIndex;

				if (!chunk->HasContent())
					return EmptyBlockIndex;

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
					Nz::Vector3ui blockIndices(x, y, z);

					Nz::Vector3f blockPos = (Nz::Vector3f(blockIndices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize + Nz::Vector3f(m_blockSize);
					Nz::Vector3f blockOffset(blockPos.x, blockPos.z, blockPos.y);

					BlockIndex blockIndex = GetBlockContent(blockIndices);

					const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);
					//if (blockData.layerIndex != layerIndex)
					//	continue;

					for (unsigned int i : { 0, 1, 2 })
					{
						auto neighboorBlockIndex = GetNeighborBlock(blockIndices, s_axis[i]);

						const auto& neighboorBlockData = m_blockLibrary.GetBlockData(neighboorBlockIndex);

						bool voxelExists = !blockData.isTransparent;
						bool neighboorExists = !neighboorBlockData.isTransparent;
						bool swapIndices = !voxelExists && blockData.isSmooth;

						if (blockData.layerIndex != layerIndex)
						{
							if (!neighboorExists || neighboorBlockData.layerIndex != layerIndex)
								continue;
						}

						std::size_t textureIndex = blockData.texIndices[Direction::Up];

						if (blockData.isTransparent)
							textureIndex = neighboorBlockData.texIndices[Direction::Up];

						//if (m_indices == ChunkIndices { 0, 2, 0 } && blockIndices == Nz::Vector3ui { 28, 12, 15 } && i == 1 && generateVisualMesh)
						//	NazaraDebugBreak();

						bool shouldGenerateFace = voxelExists != neighboorExists;
						if (!shouldGenerateFace)
						{
							if (generateVisualMesh)
							{
								// don't render faces between blocks of the same type even if transparent
								if (blockIndex != neighboorBlockIndex)
									shouldGenerateFace = blockData.isTransparent || neighboorBlockData.isTransparent;
							}
							else
							{
								if (blockIndex != neighboorBlockIndex && !blockData.hasCollisions)
									shouldGenerateFace = neighboorBlockData.hasCollisions;
							}
						}

						if (shouldGenerateFace)
						{
							VertexAttributes vertexAttributes = addFace(blockIndices, Direction::Up);

							for (unsigned int j = 0; j < 4; ++j)
							{
								bool isSmooth = blockData.isSmooth;

								Nz::Vector3f total = Nz::Vector3f::Zero();
								unsigned int count = 0;

								if (isSmooth)
								{
									for (int z = 0; z < 12; ++z)
									{
										Nz::Vector3i edgePos = s_voxelQuads[i][j] + s_edgeOffsets[z][0];
										Nz::Vector3i edgeNeighborPos = s_voxelQuads[i][j] + s_edgeOffsets[z][1];

										BlockIndex edge1 = GetNeighborBlock(blockIndices, edgePos);
										BlockIndex edge2 = GetNeighborBlock(blockIndices, edgeNeighborPos);

										const auto& edge1BlockData = m_blockLibrary.GetBlockData(edge1);
										const auto& edge2BlockData = m_blockLibrary.GetBlockData(edge2);

										if (!edge2BlockData.isSmooth)
										{
											isSmooth = false;
											break;
										}

										bool isEdge1Visible = !edge1BlockData.isTransparent;
										bool isEdge2Visible = !edge2BlockData.isTransparent;

										bool processEdge = isEdge1Visible != isEdge2Visible;
										if (generateVisualMesh)
										{
											if (edge1 != edge2)
											{
												processEdge |= edge1BlockData.isTransparent;
												processEdge |= edge2BlockData.isTransparent;
											}
										}

										if (processEdge)
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

								if (isSmooth)
									vertexAttributes.position[j] = blockOffset + m_blockSize * total / float(count) - Nz::Vector3f(m_blockSize * 0.5f);
								else
								{
									Nz::Vector3f offset(s_voxelQuads[i][j]);
									offset = { offset.x, offset.z, offset.y };

									vertexAttributes.position[j] = blockOffset + m_blockSize * offset;
								}
							}

							if (vertexAttributes.uv)
							{
								float sliceIndex = textureIndex;
								vertexAttributes.uv[0] = { 0.f, 0.f, sliceIndex };
								vertexAttributes.uv[1] = { 0.f, 0.f, sliceIndex };
								vertexAttributes.uv[2] = { 1.f, 0.f, sliceIndex };
								vertexAttributes.uv[3] = { 1.f, 0.f, sliceIndex };
							}

							// Split the quad into two triangles
							if (!swapIndices)
								indices.insert(indices.end(), { vertexAttributes.firstIndex + 0, vertexAttributes.firstIndex + 1, vertexAttributes.firstIndex + 2, vertexAttributes.firstIndex + 0, vertexAttributes.firstIndex + 2, vertexAttributes.firstIndex + 3 });
							else
								indices.insert(indices.end(), { vertexAttributes.firstIndex + 0, vertexAttributes.firstIndex + 2, vertexAttributes.firstIndex + 1, vertexAttributes.firstIndex + 0, vertexAttributes.firstIndex + 3, vertexAttributes.firstIndex + 2 });

							if (blockData.isDoubleSided)
							{
								VertexAttributes doubleSideVertexAttributes = addFace(blockIndices, Direction::Up);
								std::copy(vertexAttributes.position, vertexAttributes.position + 4, doubleSideVertexAttributes.position);

								if (vertexAttributes.uv)
									std::copy(vertexAttributes.uv, vertexAttributes.uv + 4, doubleSideVertexAttributes.uv);

								if (!swapIndices)
									indices.insert(indices.end(), { doubleSideVertexAttributes.firstIndex + 0, doubleSideVertexAttributes.firstIndex + 2, doubleSideVertexAttributes.firstIndex + 1, doubleSideVertexAttributes.firstIndex + 0, doubleSideVertexAttributes.firstIndex + 3, doubleSideVertexAttributes.firstIndex + 2 });
								else
									indices.insert(indices.end(), { doubleSideVertexAttributes.firstIndex + 0, doubleSideVertexAttributes.firstIndex + 1, doubleSideVertexAttributes.firstIndex + 2, doubleSideVertexAttributes.firstIndex + 0, doubleSideVertexAttributes.firstIndex + 2, doubleSideVertexAttributes.firstIndex + 3 });
							}
						}
					}
				}
			}
		}
	}

	Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> SurfaceNetsChunk::ComputeBlockCorners(const Nz::Vector3ui& indices) const
	{
		Nz::Vector3f blockPos = (Nz::Vector3f(indices) - Nz::Vector3f(m_size) * 0.5f) * m_blockSize;
		Nz::Vector3f blockOffset(blockPos.x, blockPos.z, blockPos.y);

		BlockIndex blockIndex = GetBlockContent(indices);

		const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

		Nz::Boxf box(blockPos.x, blockPos.z, blockPos.y, m_blockSize, m_blockSize, m_blockSize);
		auto corners = box.GetCorners();
		
		/*auto GetNeighborBlock = [&](Nz::Vector3ui indices, Nz::Vector3i direction) -> BlockIndex
		{
			ChunkIndices chunkIndices = m_indices;
			std::swap(chunkIndices.y, chunkIndices.z);

			for (unsigned int axis : { 0, 1, 2 })
			{
				unsigned int& index = indices[axis];
				int offset = direction[axis];
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
				return EmptyBlockIndex;
			}
			else
				return GetBlockContent(indices);
		};

		if (indices.z > 0)
		{
			Nz::Vector3ui baseIndices = { indices.x, indices.y, indices.z - 1 };
			BlockIndex topBlockIndex = GetBlockContent(baseIndices);

			for (unsigned int j = 0; j < 4; ++j)
			{
				bool isSmooth = blockData.isSmooth;

				Nz::Vector3f total = Nz::Vector3f::Zero();
				unsigned int count = 0;

				if (isSmooth)
				{
					for (int z = 0; z < 12; ++z)
					{
						Nz::Vector3i edgePos = s_voxelQuads[2][j] + s_edgeOffsets[z][0];
						Nz::Vector3i edgeNeighborPos = s_voxelQuads[2][j] + s_edgeOffsets[z][1];

						BlockIndex edge1 = GetNeighborBlock(baseIndices, edgePos);
						BlockIndex edge2 = GetNeighborBlock(baseIndices, edgeNeighborPos);

						const auto& edge1BlockData = m_blockLibrary.GetBlockData(edge1);
						const auto& edge2BlockData = m_blockLibrary.GetBlockData(edge2);

						if (!edge2BlockData.isSmooth)
						{
							isSmooth = false;
							break;
						}

						bool isEdge1Visible = edge1 != EmptyBlockIndex;
						bool isEdge2Visible = edge2 != EmptyBlockIndex;

						bool processEdge = (edge1 != EmptyBlockIndex) != (edge2 != EmptyBlockIndex);
						if (edge1 != edge2)
						{
							processEdge |= edge1BlockData.isTransparent;
							processEdge |= edge2BlockData.isTransparent;
						}

						if (processEdge)
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

				std::array indices = { Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::RightTopFar };

				if (isSmooth)
					corners[indices[j]] = blockOffset + m_blockSize * total / float(count) - Nz::Vector3f(m_blockSize * 0.5f);
				else
				{
					Nz::Vector3f offset(s_voxelQuads[2][j]);
					offset = { offset.x, offset.z, offset.y };

					corners[indices[j]] = blockOffset + m_blockSize * offset;
				}
			}
		}*/

		return corners;

		/*for (unsigned int i : { 0, 1, 2 })
		{
			bool voxelExists = blockIndex != EmptyBlockIndex;
			auto neighboorBlockIndex = GetNeighborBlock(blockIndices, s_axis[i]);
			bool neighboorExists = neighboorBlockIndex != EmptyBlockIndex;
			bool swapIndices = !voxelExists && blockData.isSmooth;

			const auto& neighboorBlockData = m_blockLibrary.GetBlockData(neighboorBlockIndex);

			bool shouldGenerateFace = voxelExists != neighboorExists;
			if (!shouldGenerateFace)
			{
				// don't render faces between blocks of the same type even if transparent
				if (blockIndex != neighboorBlockIndex)
					shouldGenerateFace = neighboorBlockData.isTransparent;
			}

			if (shouldGenerateFace)
			{
				VertexAttributes vertexAttributes = addFace(blockIndices, Direction::Up);

				for (unsigned int j = 0; j < 4; ++j)
				{
					bool isSmooth = blockData.isSmooth;

					Nz::Vector3f total = Nz::Vector3f::Zero();
					unsigned int count = 0;

					if (isSmooth)
					{
						for (int z = 0; z < 12; ++z)
						{
							Nz::Vector3i edgePos = s_voxelQuads[i][j] + s_edgeOffsets[z][0];
							Nz::Vector3i edgeNeighborPos = s_voxelQuads[i][j] + s_edgeOffsets[z][1];

							BlockIndex edge1 = GetNeighborBlock(blockIndices, edgePos);
							BlockIndex edge2 = GetNeighborBlock(blockIndices, edgeNeighborPos);

							const auto& edge1BlockData = m_blockLibrary.GetBlockData(edge1);
							const auto& edge2BlockData = m_blockLibrary.GetBlockData(edge2);

							if (!edge2BlockData.isSmooth)
							{
								isSmooth = false;
								break;
							}

							bool isEdge1Visible = edge1 != EmptyBlockIndex;
							bool isEdge2Visible = edge2 != EmptyBlockIndex;

							bool processEdge = (edge1 != EmptyBlockIndex) != (edge2 != EmptyBlockIndex);
							if (generateVisualMesh)
							{
								if (edge1 != edge2)
								{
									processEdge |= edge1BlockData.isTransparent;
									processEdge |= edge2BlockData.isTransparent;
								}
							}

							if (processEdge)
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

					if (isSmooth)
						vertexAttributes.position[j] = blockOffset + m_blockSize * total / float(count) - Nz::Vector3f(m_blockSize * 0.5f);
					else
					{
						Nz::Vector3f offset(s_voxelQuads[i][j]);
						offset = { offset.x, offset.z, offset.y };

						vertexAttributes.position[j] = blockOffset + m_blockSize * offset;
					}
				}
			}
		}*/
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
		Nz::Vector3f blockPos = hitPos - hitNormal * m_blockSize * 0.25f;
		std::optional<Nz::Vector3ui> blockIndices = ComputeCoordinates(blockPos);
		if (!blockIndices)
			return std::nullopt;

		return HitBlock{
			.direction = DirectionFromNormal(hitNormal),
			.blockIndices = *blockIndices
		};
	}
}
