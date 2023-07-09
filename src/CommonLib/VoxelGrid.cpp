// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/VoxelGrid.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Utility/Mesh.hpp>
#include <random>

namespace tsom
{
	VoxelGrid::VoxelGrid(std::size_t width, std::size_t height, VoxelCell defaultCell) :
	m_height(height),
	m_width(width),
	m_cells(width * height, defaultCell)
	{
		if (width > 2 && height > 2)
		{
			std::size_t x = m_width / 2;
			std::size_t y = m_height / 2;

			//m_cells[y * m_width + x] = VoxelCell::Empty;
#if 0
			std::random_device rd;
			std::uniform_int_distribution<std::size_t> disX(0, width - 1);
			std::uniform_int_distribution<std::size_t> disY(0, height - 1);
			for (std::size_t i = 0; i < m_cells.size() / 4; ++i)
			{
				std::size_t x = disX(rd);
				std::size_t y = disY(rd);
				m_cells[y * width + x] = VoxelCell::Empty;
			}
#endif
		}
	}

	void VoxelGrid::BuildMesh(const Nz::Matrix4f& transformMatrix, float cellSize, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		std::random_device rd;
		std::bernoulli_distribution dis(0.9);

		auto DrawFace = [&](Direction direction, VoxelCell cell, const std::array<Nz::Vector3f, 4>& pos, bool reverseWinding)
		{
			constexpr Nz::Vector2ui tileCount(3, 2);
			constexpr Nz::Vector2f tilesetSize(192.f, 128.f);
			constexpr Nz::Vector2f uvSize = Nz::Vector2f(64.f, 64.f) / tilesetSize;

			std::size_t tileIndex;
			switch (cell)
			{
				case VoxelCell::Dirt:
					tileIndex = 3;
					break;

				case VoxelCell::Grass:
				{
					if (direction == Direction::Up)
						tileIndex = 5;
					else if (direction == Direction::Down)
						tileIndex = 3;
					else
						tileIndex = 4;

					break;
				}

				case VoxelCell::Stone:
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
				vertices.push_back({ transformMatrix * pos[i], color, uvs[i] });

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

		for (std::size_t y = 0; y < m_height; ++y)
		{
			for (std::size_t x = 0; x < m_width; ++x)
			{
				VoxelCell cell = GetCellContent(x, y);
				if (cell == VoxelCell::Empty)
					continue;

				Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners(x, y, cellSize);

				// Up
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 0, 1); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Up, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop] }, false);
#if 1
				// Down
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 0, -1); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Down, cell, { corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

				// Front
				if (auto neighborOpt = GetNeighborCell(x, y, 0, -1, 0); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Front, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightBottom] }, true);

				// Back
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 1, 0); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Back, cell, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] }, false);

				// Left
				if (auto neighborOpt = GetNeighborCell(x, y, -1, 0, 0); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Left, cell, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] }, false);

				// Right
				if (auto neighborOpt = GetNeighborCell(x, y, 1, 0, 0); !neighborOpt || neighborOpt == VoxelCell::Empty)
					DrawFace(Direction::Right, cell, { corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::NearRightBottom] }, true);
#endif
			}
		}
	}
}
