// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNK_HPP
#define TSOM_COMMONLIB_CHUNK_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/Color.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <NazaraUtils/Signal.hpp>
#include <NazaraUtils/SparsePtr.hpp>
#include <array>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <span>
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

	class TSOM_COMMONLIB_API Chunk : public std::enable_shared_from_this<Chunk>
	{
		public:
			struct Layer;
			struct HitBlock;
			struct VertexAttributes;

			inline Chunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float blockSize);
			Chunk(const Chunk&) = delete;
			Chunk(Chunk&&) = delete;
			virtual ~Chunk();

			virtual std::pair<std::shared_ptr<Nz::Collider3D>, Nz::Vector3f> BuildBlockCollider(const Nz::Vector3ui& blockIndices, float scale = 1.f) const = 0;
			virtual std::shared_ptr<Nz::Collider3D> BuildCollider(std::size_t layerIndex) const = 0;
			virtual void BuildMesh(std::size_t layerIndex, std::vector<Nz::UInt32>& indices, const Nz::Vector3f& center, const Nz::FunctionRef<VertexAttributes(const Nz::Vector3ui& blockIndices, Direction direction)>& addFace) const;

			virtual std::optional<HitBlock> ComputeHitCoordinates(const Nz::Vector3f& hitPos, const Nz::Vector3f& hitNormal, const Nz::Collider3D& collider, std::uint32_t hitSubshapeId) const = 0;
			virtual Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeBlockCorners(const Nz::Vector3ui& indices) const;

			virtual void DeformNormals(Nz::SparsePtr<Nz::Vector3f> normals, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const;
			virtual void DeformNormalsAndTangents(Nz::SparsePtr<Nz::Vector3f> normals, Nz::SparsePtr<Nz::Vector3f> tangents, const Nz::Vector3f& referenceNormal, Nz::SparsePtr<const Nz::Vector3f> positions, std::size_t vertexCount) const;
			virtual bool DeformPositions(Nz::SparsePtr<Nz::Vector3f> positions, std::size_t positionCount) const;

			virtual void Deserialize(Nz::ByteStream& byteStream);

			inline std::span<const std::size_t> GetActiveLayers() const;
			inline Nz::UInt32 GetActiveLayerMask() const;
			inline const Nz::Bitset<Nz::UInt64>& GetCollisionCellMask(std::size_t layerIndex) const;
			inline const BlockLibrary& GetBlockLibrary() const;
			inline unsigned int GetBlockLocalIndex(const Nz::Vector3ui& indices) const;
			inline Nz::Vector3ui GetBlockLocalIndices(unsigned int blockIndex) const;
			inline BlockIndex GetBlockContent(unsigned int blockIndex) const;
			inline BlockIndex GetBlockContent(const Nz::Vector3ui& indices) const;
			inline std::size_t GetBlockCount() const;
			inline Nz::UInt16 GetBlockCount(std::size_t blockIndex) const;
			inline float GetBlockSize() const;
			inline ChunkContainer& GetContainer();
			inline const ChunkContainer& GetContainer() const;
			inline const BlockIndex* GetContent() const;
			inline const ChunkIndices& GetIndices() const;
			inline const Nz::Vector3ui& GetSize() const;

			inline bool HasContent() const;
			inline bool HasPerFaceCollisions() const;

			inline bool IsLayerRegistered(std::size_t layerIndex) const;

			inline void LockRead() const;
			inline void LockWrite();

			inline void Reset();
			template<typename F> void Reset(F&& func);

			virtual void Serialize(Nz::ByteStream& byteStream) const;

			inline void UnlockRead() const;
			inline void UnlockWrite();

			void UpdateBlock(const Nz::Vector3ui& indices, BlockIndex cellType, bool ensureContent = false);

			Chunk& operator=(const Chunk&) = delete;
			Chunk& operator=(Chunk&&) = delete;

			NazaraSignal(OnBlockUpdated, Chunk* /*emitter*/, const Nz::Vector3ui& /*indices*/, BlockIndex /*newBlock*/, std::size_t /*prevLayerIndex*/, std::size_t /*newLayerIndex*/);
			NazaraSignal(OnLayerRegistered, Chunk* /*emitter*/, std::size_t /*layerIndex*/);
			NazaraSignal(OnLayerUnregistered, Chunk* /*emitter*/, std::size_t /*layerIndex*/);
			NazaraSignal(OnReset, Chunk* /*emitter*/);

			struct Layer
			{
				Nz::Bitset<Nz::UInt64> collisionCellMasks;
				std::size_t blockCount = 0; //< Number of blocks on this layer
			};

			struct HitBlock
			{
				Direction direction;
				Nz::Vector3ui blockIndices;
			};

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
			inline void RegisterLayer(std::size_t layerIndex);
			inline void SetPerFaceCollision();
			inline void UnregisterLayer(std::size_t layerIndex);

			mutable std::shared_mutex m_mutex;
			std::array<std::optional<Layer>, Constants::MaxChunkLayerCount> m_layers;
			std::vector<BlockIndex> m_blocks;
			std::vector<Nz::UInt16> m_blockTypeCount;
			Nz::FixedVector<std::size_t, Constants::MaxChunkLayerCount> m_activeLayers;
			Nz::Vector3ui m_size;
			ChunkIndices m_indices;
			const BlockLibrary& m_blockLibrary;
			ChunkContainer& m_owner;
			bool m_hasPerFaceCollision;
			float m_blockSize;
	};
}

#include <CommonLib/Chunk.inl>

#endif // TSOM_COMMONLIB_CHUNK_HPP
