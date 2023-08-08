// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientPlanetEntities.hpp>
#include <ClientLib/ClientPlanet.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/AppFilesystemComponent.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Utility/IndexBuffer.hpp>
#include <Nazara/Utility/VertexBuffer.hpp>

namespace tsom
{
	ClientPlanetEntities::ClientPlanetEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientPlanet& planet) :
	PlanetEntities(world, planet, NoInit{})
	{
		auto& filesystem = app.GetComponent<Nz::AppFilesystemComponent>();

		Nz::TextureSamplerInfo blockSampler;
		blockSampler.anisotropyLevel = 16;
		blockSampler.magFilter = Nz::SamplerFilter::Nearest;
		blockSampler.minFilter = Nz::SamplerFilter::Nearest;
		blockSampler.wrapModeU = Nz::SamplerWrap::Repeat;
		blockSampler.wrapModeV = Nz::SamplerWrap::Repeat;

		m_chunkMaterial = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
		m_chunkMaterial->SetTextureProperty("BaseColorMap", filesystem.Load<Nz::Texture>("assets/tileset.png"), blockSampler);

		FillChunks();
	}

	std::shared_ptr<Nz::Model> ClientPlanetEntities::BuildModel(const Chunk* chunk)
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		chunk->BuildMesh(indices, vertices);

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, Nz::SafeCast<Nz::UInt32>(indices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Color_UV), Nz::SafeCast<Nz::UInt32>(vertices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();

		Nz::Mesh chunkMesh;
		chunkMesh.CreateStatic();
		chunkMesh.AddSubMesh(std::move(staticMesh));

		std::shared_ptr<Nz::GraphicalMesh> gfxMesh = Nz::GraphicalMesh::BuildFromMesh(chunkMesh);

		std::shared_ptr<Nz::Model> model = std::make_shared<Nz::Model>(std::move(gfxMesh));
		model->SetMaterial(0, m_chunkMaterial);

		return model;
	}

	void ClientPlanetEntities::CreateChunkEntity(std::size_t chunkId, const Chunk* chunk)
	{
		PlanetEntities::CreateChunkEntity(chunkId, chunk);

		std::shared_ptr<Nz::Model> model = BuildModel(chunk);

		m_chunkEntities[chunkId].emplace<Nz::GraphicsComponent>(std::move(model), 0x0000FFFF);
	}

	void ClientPlanetEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		PlanetEntities::UpdateChunkEntity(chunkId);

		std::shared_ptr<Nz::Model> model = BuildModel(m_planet.GetChunk(chunkId));

		auto& gfxComponent = m_chunkEntities[chunkId].get_or_emplace<Nz::GraphicsComponent>();
		gfxComponent.Clear();
		gfxComponent.AttachRenderable(std::move(model), 0x0000FFFF);
	}
}
