// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNK_HPP
#define TSOM_COMMONLIB_CHUNK_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Direction.hpp>
#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Matrix4.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <NazaraUtils/Signal.hpp>
#include <NazaraUtils/SparsePtr.hpp>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace Nz
{
	class ByteStream;
	class Collider3D;
	struct VertexStruct_XYZ_Color_UV;
}

namespace tsom
{
	class BlockLibrary;
	class ChunkContainer;

	using BlockIndices = Nz::Vector3i32;
	using ChunkIndices = Nz::Vector3i32;

	class TSOM_COMMONLIB_API Chunk
	{
		public:
			struct VertexAttributes;

			inline Chunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float blockSize);
			Chunk(const Chunk&) = delete;
			Chunk(Chunk&&) = delete;
			virtual ~Chunk();

			virtual std::shared_ptr<Nz::Collider3D> BuildCollider() const = 0;
			virtual void BuildMesh(std::vector<Nz::UInt32>& indices, const Nz::Vector3f& center, const Nz::FunctionRef<VertexAttributes(Nz::UInt32 count)>& addVertices) const;

			virtual std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const = 0;
			virtual Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(const Nz::Vector3ui& indices) const;

			virtual void DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const;
			virtual void DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const;
			virtual bool DeformPositions(Nz::SparsePtr<Nz::Vector3f> positions, std::size_t positionCount) const;

			virtual void Deserialize(const BlockLibrary& blockLibrary, Nz::ByteStream& byteStream);

			inline const Nz::Bitset<Nz::UInt64>& GetCollisionCellMask() const;
			inline unsigned int GetBlockLocalIndex(const Nz::Vector3ui& indices) const;
			inline Nz::Vector3ui GetBlockLocalIndices(unsigned int blockIndex) const;
			inline BlockIndex GetBlockContent(unsigned int blockIndex) const;
			inline BlockIndex GetBlockContent(const Nz::Vector3ui& indices) const;
			inline std::size_t GetBlockCount() const;
			inline float GetBlockSize() const;
			inline ChunkContainer& GetContainer();
			inline const ChunkContainer& GetContainer() const;
			inline const BlockIndex* GetContent() const;
			inline const ChunkIndices& GetIndices() const;
			inline const Nz::Vector3ui& GetSize() const;

			inline bool HasContent() const;

			inline void LockRead() const;
			inline void LockWrite();

			inline void Reset();
			template<typename F> void Reset(F&& func);

			virtual void Serialize(const BlockLibrary& blockLibrary, Nz::ByteStream& byteStream);

			inline void UnlockRead() const;
			inline void UnlockWrite();

			void UpdateBlock(const Nz::Vector3ui& indices, BlockIndex cellType);

			Chunk& operator=(const Chunk&) = delete;
			Chunk& operator=(Chunk&&) = delete;

			NazaraSignal(OnBlockUpdated, Chunk* /*emitter*/, const Nz::Vector3ui& /*indices*/, BlockIndex /*newBlock*/);
			NazaraSignal(OnReset, Chunk* /*emitter*/);

			struct VertexAttributes
			{
				Nz::UInt32 firstIndex;
				Nz::SparsePtr<Nz::Color> color;
				Nz::SparsePtr<Nz::Vector3f> position;
				Nz::SparsePtr<Nz::Vector3f> normal;
				Nz::SparsePtr<Nz::Vector3f> tangent;
				Nz::SparsePtr<Nz::Vector3f> uv;
			};

		protected:
			void OnChunkReset();

			mutable std::shared_mutex m_mutex;
			std::vector<BlockIndex> m_blocks;
			std::vector<Nz::UInt16> m_blockTypeCount;
			Nz::Bitset<Nz::UInt64> m_collisionCellMask;
			Nz::Vector3ui m_size;
			ChunkIndices m_indices;
			const BlockLibrary& m_blockLibrary;
			ChunkContainer& m_owner;
			float m_blockSize;
	};
}

#include <CommonLib/Chunk.inl>

#endif // TSOM_COMMONLIB_CHUNK_HPP
