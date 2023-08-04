// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Chunk.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <random>

namespace tsom
{
	void Chunk::BuildMesh(const Nz::Matrix4f& transformMatrix, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices) const
	{
		std::random_device rd;
		std::bernoulli_distribution dis(0.9);

		auto DrawFace = [&](Direction direction, VoxelBlock cell, const std::array<Nz::Vector3f, 4>& pos, bool reverseWinding)
		{
			constexpr Nz::Vector2ui tileCount(3, 2);
			constexpr Nz::Vector2f tilesetSize(192.f, 128.f);
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

				case VoxelBlock::Stone:
					tileIndex = (dis(rd)) ? 1 : 2;
					break;
			}

			Nz::Vector2ui tileCoords(tileIndex % tileCount.x, tileIndex / tileCount.x);
			Nz::Vector2f uv(tileCoords);
			uv *= uvSize;

			std::array uvs = {
				Nz::Vector2f(uv.x, uv.y),
				Nz::Vector2f(uv.x + uvSize.x, uv.y),
				Nz::Vector2f(uv.x, uv.y + uvSize.y),
				Nz::Vector2f(uv.x + uvSize.x, uv.y + uvSize.y)
			};

			std::size_t n = vertices.size();
			for (std::size_t i = 0; i < 4; ++i)
				vertices.push_back({ transformMatrix * pos[i], Nz::Color::White(), uvs[i]});

			if (reverseWinding)
			{
				indices.push_back(n);
				indices.push_back(n + 1);
				indices.push_back(n + 2);

				indices.push_back(n + 2);
				indices.push_back(n + 1);
				indices.push_back(n + 3);
			}
			else
			{
				indices.push_back(n);
				indices.push_back(n + 2);
				indices.push_back(n + 1);

				indices.push_back(n + 1);
				indices.push_back(n + 2);
				indices.push_back(n + 3);
			}
		};

		for (unsigned int z = 0; z < m_size.z; ++z)
		{
			for (unsigned int y = 0; y < m_size.y; ++y)
			{
				for (unsigned int x = 0; x < m_size.x; ++x)
				{
					VoxelBlock cell = GetCellContent({ x, y, z });
					if (cell == VoxelBlock::Empty)
						continue;

					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners({ x, y, z });

					// Up
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { 0, 0, 1 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Up, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop] }, false);

					// Down
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { 0, 0, -1 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Down, cell, { corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Front
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { 0, -1, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Front, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

					// Back
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { 0, 1, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Back, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] }, false);

					// Left
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { -1, 0, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Left, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] }, false);

					// Right
					if (auto neighborOpt = GetNeighborCell({ x, y, z }, { 1, 0, 0 }); !neighborOpt || neighborOpt == VoxelBlock::Empty)
						DrawFace(Direction::Right, cell, { corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::NearRightBottom] }, true);
				}
			}
		}
	}
}
