// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Chunk.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <cassert>

namespace tsom
{
	Chunk::~Chunk() = default;

	void Chunk::BuildMesh(std::vector<Nz::UInt32>& indices, const Nz::FunctionRef<VertexAttributes(Nz::UInt32)>& addVertices) const
	{
		auto DrawFace = [&](Direction direction, VoxelBlock cell, const std::array<Nz::Vector3f, 4>& pos, bool reverseWinding)
		{
			VertexAttributes vertexAttributes = addVertices(pos.size());
			assert(vertexAttributes.position);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = s_dirNormals[direction];
			}

			if (vertexAttributes.uv)
			{
				constexpr Nz::Vector2ui tileCount(3, 3);
				constexpr Nz::Vector2f tilesetSize(192.f, 192.f);
				constexpr Nz::Vector2f uvSize = Nz::Vector2f(64.f, 64.f) / tilesetSize;

				std::size_t tileIndex;
				switch (cell)
				{
					case VoxelBlock::Dirt:
						tileIndex = 3;
						break;

					case VoxelBlock::Grass:
					{
						if (direction == Direction::Up)
							tileIndex = 5;
						else if (direction == Direction::Down)
							tileIndex = 3;
						else
							tileIndex = 4;

						break;
					}

					case VoxelBlock::MossedStone:
						tileIndex = 2;
						break;

					case VoxelBlock::Snow:
						tileIndex = 6;
						break;

					case VoxelBlock::Stone:
						tileIndex = 1;
						break;
				}

				Nz::Vector2ui tileCoords(tileIndex % tileCount.x, tileIndex / tileCount.x);
				Nz::Vector2f uv(tileCoords);
				uv *= uvSize;

				vertexAttributes.uv[0] = Nz::Vector2f(uv.x, uv.y);
				vertexAttributes.uv[1] = Nz::Vector2f(uv.x + uvSize.x, uv.y);
				vertexAttributes.uv[2] = Nz::Vector2f(uv.x, uv.y + uvSize.y);
				vertexAttributes.uv[3] = Nz::Vector2f(uv.x + uvSize.x, uv.y + uvSize.y);
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
					VoxelBlock cell = GetBlockContent({ x, y, z });
					if (cell == VoxelBlock::Empty)
						continue;

					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners({ x, y, z });

					// Up
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, 1 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Up, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop] }, false);

					// Down
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, -1 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Down, cell, { corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Front
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, -1, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Front, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Back
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 1, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Back, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] }, false);

					// Left
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { -1, 0, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Left, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] }, false);

					// Right
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 1, 0, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Right, cell, { corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::NearRightBottom] }, true);
				}
			}
		}
	}
}
