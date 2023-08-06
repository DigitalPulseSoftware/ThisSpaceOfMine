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
	m_chunkCount(Nz::Vector3ui((gridSize + Nz::Vector3ui(ChunkSize - 1)) / ChunkSize)),
	m_gridSize(gridSize),
	m_cornerRadius(cornerRadius),
	m_tileSize(tileSize)
	{
		m_chunks.resize(m_chunkCount.x * m_chunkCount.y * m_chunkCount.z);
	}

	Chunk& Planet::AddChunk(const Nz::Vector3ui& indices)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(!m_chunks[index]);
		m_chunks[index] = std::make_unique<FlatChunk>(indices, Nz::Vector3ui{ ChunkSize }, m_tileSize);
		return *m_chunks[index];
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
					auto& chunkPtr = m_chunks[m_chunkCount.x * (z * m_chunkCount.y + y) + x];
					if (!chunkPtr)
						continue;

					auto collider = chunkPtr->BuildCollider();
					if (!collider)
						continue;

					auto& colliderEntry = colliders.emplace_back();
					colliderEntry.collider = std::move(collider);
					colliderEntry.offset = GetChunkOffset({ x, y, z });
				}
			}
		}

		if (colliders.empty())
			return nullptr;

		return std::make_shared<Nz::JoltCompoundCollider3D>(std::move(colliders));
	}

	void Planet::GenerateChunks()
	{
		constexpr std::size_t freeSpace = 10;

		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
					AddChunk({ x, y, z });
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
					chunk.UpdateBlock(innerCoordinates, VoxelBlock::Grass);
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

	void Planet::RemoveChunk(const Nz::Vector3ui& indices)
	{
		std::size_t index = GetChunkIndex(indices);
		assert(m_chunks[index]);
		m_chunks[index] = nullptr;
	}

	void Planet::BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		for (unsigned int z = 0; z < m_chunkCount.z; ++z)
		{
			for (unsigned int y = 0; y < m_chunkCount.y; ++y)
			{
				for (unsigned int x = 0; x < m_chunkCount.x; ++x)
				{
					auto& chunkPtr = m_chunks[m_chunkCount.x * (z * m_chunkCount.y + y) + x];
					if (chunkPtr)
						chunkPtr->BuildMesh(Nz::Matrix4f::Translate(GetChunkOffset({ x, y, z })), indices, vertices);
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
}
