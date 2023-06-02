// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Client/VoxelGrid.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Utility/Mesh.hpp>
#include <random>

namespace tsom
{
	VoxelGrid::VoxelGrid(std::size_t width, std::size_t height) :
	m_height(height),
	m_width(width),
	m_cells(width * height, VoxelCell::Full)
	{
		if (width > 2 && height > 2)
		{
			std::size_t x = m_width / 2;
			std::size_t y = m_height / 2;

			m_cells[y * m_width + x] = VoxelCell::Empty;

			/*std::random_device rd;
			std::uniform_int_distribution<std::size_t> disX(0, width - 1);
			std::uniform_int_distribution<std::size_t> disY(0, height - 1);
			for (std::size_t i = 0; i < m_cells.size() / 4; ++i)
			{
				std::size_t x = disX(rd);
				std::size_t y = disY(rd);
				m_cells[y * width + x] = VoxelCell::Empty;
			}*/
		}
	}

	void VoxelGrid::BuildMesh(const Nz::Matrix4f& transformMatrix, float cellSize, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		auto DrawFace = [&](const std::array<Nz::Vector3f, 4>& pos)
		{
			constexpr std::array uvs = {
				Nz::Vector2f(0.f, 0.f),
				Nz::Vector2f(1.f, 0.f),
				Nz::Vector2f(0.f, 1.f),
				Nz::Vector2f(1.f, 1.f)
			};

			std::size_t n = vertices.size();
			for (std::size_t i = 0; i < 4; ++i)
				vertices.push_back({ transformMatrix * pos[i], color, uvs[i] });

			indices.push_back(n);
			indices.push_back(n + 2);
			indices.push_back(n + 1);

			indices.push_back(n + 1);
			indices.push_back(n + 2);
			indices.push_back(n + 3);
		};

		for (std::size_t y = 0; y < m_height; ++y)
		{
			for (std::size_t x = 0; x < m_width; ++x)
			{
				if (GetCellContent(x, y) == VoxelCell::Empty)
					continue;

				float fX = x * cellSize;
				float fY = y * cellSize;

				Nz::Boxf box(fX, 0.f, fY, cellSize, cellSize, cellSize);
				Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = {
					box.GetCorner(Nz::BoxCorner::FarLeftBottom),
					box.GetCorner(Nz::BoxCorner::FarLeftTop),
					box.GetCorner(Nz::BoxCorner::FarRightBottom),
					box.GetCorner(Nz::BoxCorner::FarRightTop),
					box.GetCorner(Nz::BoxCorner::NearLeftBottom),
					box.GetCorner(Nz::BoxCorner::NearLeftTop),
					box.GetCorner(Nz::BoxCorner::NearRightBottom),
					box.GetCorner(Nz::BoxCorner::NearRightTop)
				};

				if (x == 0)
				{
					corners[Nz::BoxCorner::NearLeftTop].x -= cellSize;
					corners[Nz::BoxCorner::FarLeftTop].x -= cellSize;
				}
				
				if (x == m_width - 1)
				{
					corners[Nz::BoxCorner::NearRightTop].x += cellSize;
					corners[Nz::BoxCorner::FarRightTop].x += cellSize;
				}

				if (y == 0)
				{
					corners[Nz::BoxCorner::FarLeftTop].z -= cellSize;
					corners[Nz::BoxCorner::FarRightTop].z -= cellSize;
				}
				
				if (y == m_height - 1)
				{
					corners[Nz::BoxCorner::NearLeftTop].z += cellSize;
					corners[Nz::BoxCorner::NearRightTop].z += cellSize;
				}

				// Up
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 0, 1); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop] });
#if 0
				// Down
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 0, -1); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightBottom] });

				// Front
				if (auto neighborOpt = GetNeighborCell(x, y, 0, -1, 0); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarRightBottom] });

				// Back
				if (auto neighborOpt = GetNeighborCell(x, y, 0, 1, 0); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] });

				// Left
				if (auto neighborOpt = GetNeighborCell(x, y, -1, 0, 0); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] });

				// Right
				if (auto neighborOpt = GetNeighborCell(x, y, 1, 0, 0); neighborOpt != VoxelCell::Full)
					DrawFace({ corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearRightBottom] });
#endif
			}
		}
	}
}
