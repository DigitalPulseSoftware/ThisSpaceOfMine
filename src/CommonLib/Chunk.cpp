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
	Chunk::~Chunk() = default;

	void Chunk::BuildMesh(const BlockLibrary& blockManager, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& gravityCenter, const Nz::FunctionRef<VertexAttributes(Nz::UInt32)>& addVertices) const
	{
		auto DrawFace = [&](BlockIndex blockIndex, const Nz::Vector3f& blockCenter, const std::array<Nz::Vector3f, 4>& pos)
		{
			VertexAttributes vertexAttributes = addVertices(pos.size());
			assert(vertexAttributes.position);

			for (std::size_t i = 0; i < pos.size(); ++i)
				vertexAttributes.position[i] = pos[i];

			Nz::Vector3f faceCenter = std::accumulate(pos.begin(), pos.end(), Nz::Vector3f::Zero()) / pos.size();
			Nz::Vector3f faceDirection = Nz::Vector3f::Normalize(faceCenter - blockCenter);

			if (vertexAttributes.normal)
			{
				for (std::size_t i = 0; i < pos.size(); ++i)
					vertexAttributes.normal[i] = faceDirection;
			}

			if (vertexAttributes.uv)
			{
				// Get face up vector
				Nz::Vector3f faceUp = s_dirNormals[DirectionFromNormal(Nz::Vector3f::Normalize(faceCenter - gravityCenter))];

				// Make up the rotation from the face up to the regular up
				Nz::Quaternionf upRotation = Nz::Quaternionf::RotationBetween(faceUp, Nz::Vector3f::Up());

				// Compute texture direction based on face direction in regular orientation
				Direction texDirection = DirectionFromNormal(upRotation * faceDirection);

				const auto& blockData = blockManager.GetBlockData(blockIndex);
				std::size_t textureIndex = blockData.texIndices[texDirection];

				// Compute UV
				float sliceIndex = textureIndex;
				for (std::size_t i = 0; i < pos.size(); ++i)
				{
					// Get vector from center to corner (no need to normalize) and use it to compute UV
					// This is similar to the way a GPU compute UV when sampling a cubemap: https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/5337472/
					Nz::Vector3f dir = upRotation * (pos[i] - blockCenter);
					Nz::Vector3f dirAbs = dir.GetAbs();

					float mag;
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

			indices.push_back(vertexAttributes.firstIndex);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 1);

			indices.push_back(vertexAttributes.firstIndex + 1);
			indices.push_back(vertexAttributes.firstIndex + 2);
			indices.push_back(vertexAttributes.firstIndex + 3);
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

					Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners = ComputeVoxelCorners({ x, y, z });

					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					// Up
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, 1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop] });

					// Down
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 0, -1 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::NearLeftBottom] });

					// Front
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, -1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::FarRightBottom], corners[Nz::BoxCorner::FarLeftBottom] });

					// Back
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 0, 1, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::NearLeftBottom], corners[Nz::BoxCorner::NearRightBottom] });

					// Left
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { -1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::FarLeftTop], corners[Nz::BoxCorner::NearLeftTop], corners[Nz::BoxCorner::FarLeftBottom], corners[Nz::BoxCorner::NearLeftBottom] });

					// Right
					if (auto neighborOpt = GetNeighborBlock({ x, y, z }, { 1, 0, 0 }); !neighborOpt || neighborOpt == EmptyBlockIndex)
						DrawFace(blockIndex, blockCenter, { corners[Nz::BoxCorner::NearRightTop], corners[Nz::BoxCorner::FarRightTop], corners[Nz::BoxCorner::NearRightBottom], corners[Nz::BoxCorner::FarRightBottom] });
				}
			}
		}
	}
}
