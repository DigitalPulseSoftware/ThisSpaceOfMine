// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Chunk.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <cassert>
#include <numeric>

namespace tsom
{
	constexpr Nz::EnumArray<Direction, Nz::EnumArray<Direction, Direction>> s_texDirections = {
		{
			// Back
			Nz::EnumArray<Direction, Direction>{
				Direction::Up,    // Direction::Back
				Direction::Back,  // Direction::Down
				Direction::Down,  // Direction::Front
				Direction::Left,  // Direction::Left
				Direction::Right, // Direction::Right
				Direction::Front, // Direction::Up
			},
			// Down
			Nz::EnumArray<Direction, Direction>{
				Direction::Front, // Direction::Back
				Direction::Up,    // Direction::Down
				Direction::Back,  // Direction::Front
				Direction::Left,  // Direction::Right
				Direction::Right, // Direction::Left
				Direction::Down,  // Direction::Up
			},
			// Front
			Nz::EnumArray<Direction, Direction>{
				Direction::Down,  // Direction::Back
				Direction::Front, // Direction::Down
				Direction::Up,    // Direction::Front
				Direction::Left,  // Direction::Right
				Direction::Right, // Direction::Left
				Direction::Back,  // Direction::Up
			},
			// Left
			Nz::EnumArray<Direction, Direction>{
				Direction::Back,  // Direction::Back
				Direction::Left,  // Direction::Down
				Direction::Front, // Direction::Front
				Direction::Up,    // Direction::Right
				Direction::Down,  // Direction::Left
				Direction::Right, // Direction::Up
			},
			// Right
			Nz::EnumArray<Direction, Direction>{
				Direction::Back,  // Direction::Back
				Direction::Right, // Direction::Down
				Direction::Front, // Direction::Front
				Direction::Down,  // Direction::Right
				Direction::Up,    // Direction::Left
				Direction::Left,  // Direction::Up
			},
			// Up
			Nz::EnumArray<Direction, Direction>{
				Direction::Back,  // Direction::Back
				Direction::Down,  // Direction::Down
				Direction::Front, // Direction::Front
				Direction::Left,  // Direction::Right
				Direction::Right, // Direction::Left
				Direction::Up,    // Direction::Up
			}
		}
	};

	Chunk::~Chunk() = default;

	void Chunk::BuildMesh(const BlockLibrary& blockManager, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& center, const Nz::FunctionRef<VertexAttributes(Nz::UInt32)>& addVertices) const
	{
		auto DrawFace = [&](Direction normalDirection, Direction upDirection, BlockIndex blockIndex, const std::array<Nz::Vector3f, 4>& pos, bool reverseWinding)
		{
			VertexAttributes vertexAttributes = addVertices(pos.size());
			assert(vertexAttributes.position);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = s_dirNormals[normalDirection];
			}

			if (vertexAttributes.uv)
			{
				Direction texDirection = s_texDirections[upDirection][normalDirection];

				const auto& blockData = blockManager.GetBlockData(blockIndex);
				std::size_t textureIndex = blockData.texIndices[texDirection];

				constexpr Nz::Vector2f uv(0.f, 0.f);
				constexpr Nz::Vector2f uvSize(1.f, 1.f);

				float sliceIndex = textureIndex;

				vertexAttributes.uv[0] = Nz::Vector3f(uv.x, uv.y, sliceIndex);
				vertexAttributes.uv[1] = Nz::Vector3f(uv.x + uvSize.x, uv.y, sliceIndex);
				vertexAttributes.uv[2] = Nz::Vector3f(uv.x, uv.y + uvSize.y, sliceIndex);
				vertexAttributes.uv[3] = Nz::Vector3f(uv.x + uvSize.x, uv.y + uvSize.y, sliceIndex);
			}

			if (reverseWinding)
			{
				indices.push_back(vertexAttributes.firstIndex);
				indices.push_back(vertexAttributes.firstIndex + 1);
				indices.push_back(vertexAttributes.firstIndex + 2);

				indices.push_back(vertexAttributes.firstIndex + 2);
				indices.push_back(vertexAttributes.firstIndex + 1);
				indices.push_back(vertexAttributes.firstIndex + 3);
			}
			else
			{
				indices.push_back(vertexAttributes.firstIndex);
				indices.push_back(vertexAttributes.firstIndex + 2);
				indices.push_back(vertexAttributes.firstIndex + 1);

				indices.push_back(vertexAttributes.firstIndex + 1);
				indices.push_back(vertexAttributes.firstIndex + 2);
				indices.push_back(vertexAttributes.firstIndex + 3);
			}
		};

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					BlockIndex cell = GetBlockContent({ x, y, z });
					if (cell == EmptyBlockIndex)
						continue;

					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners({ x, y, z });

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();
					Direction direction = DirectionFromNormal(Nz::Vector3f::Normalize(blockCenter - center));

					// Up
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, 1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Up, direction, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop] }, false);

					// Down
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, -1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Down, direction, cell, { corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Front
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, -1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Front, direction, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Back
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Back, direction, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] }, false);

					// Left
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { -1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Left, direction, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] }, false);

					// Right
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(Direction::Right, direction, cell, { corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::NearRightBottom] }, true);
				}
			}
		}
	}
}
