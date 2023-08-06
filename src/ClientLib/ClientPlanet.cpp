// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientPlanet.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>

namespace tsom
{
	ClientPlanet::ClientPlanet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius) :
	Planet(gridSize, tileSize, cornerRadius)
	{
	}

	Chunk& ClientPlanet::AddChunk(Nz::UInt16 networkIndex, const Nz::Vector3ui& indices)
	{
		Chunk& chunk = Planet::AddChunk(indices);

		m_chunkByNetworkIndex.emplace(networkIndex, &chunk);
		m_chunkNetworkIndices.emplace(&chunk, networkIndex);

		return chunk;
	}

	void ClientPlanet::RemoveChunk(Nz::UInt16 networkIndex)
	{
		Chunk* chunk = Nz::Retrieve(m_chunkByNetworkIndex, networkIndex);

		m_chunkNetworkIndices.erase(chunk);
		m_chunkByNetworkIndex.erase(networkIndex);

		Planet::RemoveChunk(chunk->GetIndices());
	}

	std::shared_ptr<Nz::GraphicalMesh> ClientPlanet::BuildGfxMesh()
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(indices, vertices);
		if (indices.empty())
			return nullptr;

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, Nz::SafeCast<Nz::UInt32>(indices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Color_UV), Nz::SafeCast<Nz::UInt32>(vertices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();
		staticMesh->SetPrimitiveMode(Nz::PrimitiveMode::LineList);

		return Nz::GraphicalMesh::BuildFromMesh(*Nz::Mesh::Build(staticMesh));
	}
}
