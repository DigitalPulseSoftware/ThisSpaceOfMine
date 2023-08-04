// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/Planet.hpp>
#include <CommonLib/DeformedChunk.hpp>
#include <CommonLib/FlatChunk.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
#include <fmt/format.h>
#include <random>

namespace tsom
{
	Planet::Planet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius) :
	m_gridSize(gridSize),
	m_cornerRadius(cornerRadius),
	m_tileSize(tileSize)
	{
		RebuildGrid();
	}

	std::shared_ptr<Nz::JoltCollider3D> Planet::BuildCollider()
	{
		std::vector<Nz::JoltCompoundCollider3D::ChildCollider> colliders;

		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
				{
					auto& colliderEntry = colliders.emplace_back();

					auto& chunk = *m_chunks[m_chunkCount.x * (z * m_chunkCount.y + y) + x];
					colliderEntry.collider = chunk.BuildCollider();
					colliderEntry.offset = GetChunkOffset({ x, y, z });
				}
			}
		}

		return std::make_shared<Nz::JoltCompoundCollider3D>(std::move(colliders));
	}

	std::optional<Nz::Vector3ui> Planet::ComputeGridCell(const Nz::Vector3f& position) const
	{
		Nz::Vector3f recenterOffset = 0.5f * Nz::Vector3f(m_chunkCount) * ChunkSize * m_tileSize;
		Nz::Vector3f indices = position + recenterOffset;
		indices /= Nz::Vector3f(ChunkSize * m_tileSize);

		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return std::nullopt;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_chunkCount.x || pos.y >= m_chunkCount.y || pos.z >= m_chunkCount.z)
			return std::nullopt;

		return m_chunks[m_chunkCount.z * (m_chunkCount.y * pos.z + pos.y) + pos.x]->ComputeCoordinates(position);
	}

	void Planet::BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
				{
					auto& chunk = *m_chunks[m_chunkCount.x * (z * m_chunkCount.y + y) + x];
					chunk.BuildMesh(Nz::Matrix4f::Translate(GetChunkOffset({ x, y, z })), indices, vertices);
				}
			}
		}

		float maxHeight = m_gridSize.y / 2 * m_tileSize;

		Nz::Vector3f center = GetCenter();
		for (Nz::VertexStruct_XYZ_Color_UV& vert : vertices)
		{
			float distToCenter = std::max({
				std::abs(vert.position.x - center.x),
				std::abs(vert.position.y - center.y),
				std::abs(vert.position.z - center.z),
			});

			vert.color *= Nz::Color(std::clamp(distToCenter / maxHeight * 2.f, 0.f, 1.f));
		}
	}

	void Planet::RebuildGrid()
	{
		constexpr std::size_t freeSpace = 10;

		m_chunkCount = Nz::Vector3ui((m_gridSize + Nz::Vector3ui(ChunkSize - 1)) / ChunkSize);
		m_chunks.resize(m_chunkCount.x * m_chunkCount.y * m_chunkCount.z);
		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
					m_chunks[z * m_chunkCount.y * m_chunkCount.x + y * m_chunkCount.x + x] = std::make_unique<FlatChunk>(Nz::Vector3ui{ x, y, z }, Nz::Vector3ui{ ChunkSize }, m_tileSize);
			}
		}

		for (unsigned int z = 0; z < m_gridSize.z; ++z)
		{
			for (unsigned int y = 0; y < m_gridSize.y; ++y)
			{
				for (unsigned int x = 0; x < m_gridSize.x; ++x)
				{
					if (x < freeSpace || x >= m_gridSize.x - freeSpace ||
						y < freeSpace || y >= m_gridSize.y - freeSpace ||
						z < freeSpace || z >= m_gridSize.z - freeSpace)
						continue;

					unsigned int depth = z - freeSpace;
					VoxelBlock blockType;
					if (depth == 0)
						blockType = VoxelBlock::Grass;
					else if (depth <= 3)
						blockType = VoxelBlock::Dirt;
					else
						blockType = VoxelBlock::Stone;

					Nz::Vector3ui innerCoordinates;
					Chunk& chunk = GetChunkByIndices({ x, y, z }, &innerCoordinates);
					chunk.UpdateCell(innerCoordinates, VoxelBlock::Grass);
				}
			}
		}

		/*for (auto& gridVec : m_grids)
		{
			gridVec.clear();

			std::size_t maxHeight = m_gridDimensions / 2;

			std::size_t gridSize = 1;
			for (std::size_t i = 0; i < m_gridDimensions; ++i)
			{
				VoxelBlock cell;
				if (i >= maxHeight)
					cell = VoxelBlock::Empty;
				else if (i == maxHeight - 1)
					cell = VoxelBlock::Grass;
				else if (i >= maxHeight - 3)
					cell = VoxelBlock::Dirt;
				else
					cell = VoxelBlock::Stone;

				gridVec.emplace_back(std::make_unique<Chunk>(gridSize, gridSize, cell));
				gridSize += 2;
			}
		}*/
	}
}
