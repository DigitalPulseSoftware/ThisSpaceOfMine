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
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
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

		m_chunkMaterial = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Phong);
		m_chunkMaterial->SetTextureProperty("BaseColorMap", filesystem.Load<Nz::Texture>("assets/tileset.png"), blockSampler);
		m_chunkMaterial->SetValueProperty("SpecularColor", Nz::Color::White() * Nz::Color(0.2f));
		m_chunkMaterial->SetValueProperty("Shininess", 10.f);

		FillChunks();
	}

	std::shared_ptr<Nz::Model> ClientPlanetEntities::BuildModel(const Chunk* chunk)
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Normal_UV_Tangent> vertices;

		auto AddVertices = [&](Nz::UInt32 count)
		{
			Chunk::VertexAttributes vertexAttributes;

			vertexAttributes.firstIndex = Nz::SafeCast<Nz::UInt32>(vertices.size());
			vertices.resize(vertices.size() + count);
			vertexAttributes.position = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].position, sizeof(vertices.front()));
			vertexAttributes.normal = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].normal, sizeof(vertices.front()));
			vertexAttributes.tangent = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].tangent, sizeof(vertices.front()));
			vertexAttributes.uv = Nz::SparsePtr<Nz::Vector2f>(&vertices[vertexAttributes.firstIndex].uv, sizeof(vertices.front()));

			return vertexAttributes;
		};

		chunk->BuildMesh(indices, AddVertices);
		if (indices.empty())
			return nullptr;

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, Nz::SafeCast<Nz::UInt32>(indices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Normal_UV_Tangent), Nz::SafeCast<Nz::UInt32>(vertices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

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

		auto& gfxComponent = m_chunkEntities[chunkId].emplace<Nz::GraphicsComponent>();
		if (model)
			gfxComponent.AttachRenderable(std::move(model), 0x0000FFFF);

		UpdateChunkDebugCollider(chunkId);
	}

	void ClientPlanetEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		PlanetEntities::UpdateChunkEntity(chunkId);

		std::shared_ptr<Nz::Model> model = BuildModel(m_planet.GetChunk(chunkId));

		auto& gfxComponent = m_chunkEntities[chunkId].get_or_emplace<Nz::GraphicsComponent>();
		gfxComponent.Clear();
		if (model)
			gfxComponent.AttachRenderable(std::move(model), 0x0000FFFF);

		UpdateChunkDebugCollider(chunkId);
	}

	void ClientPlanetEntities::UpdateChunkDebugCollider(std::size_t chunkId)
	{
#if 0
		std::shared_ptr<Nz::Model> colliderModel;
		{
			auto& rigidBodyComponent = m_chunkEntities[chunkId].get<Nz::JoltRigidBody3DComponent>();
			const std::shared_ptr<Nz::JoltCollider3D>& geom = rigidBodyComponent.GetGeom();
			if (!geom)
				return;

			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(geom->GenerateDebugMesh());
			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

			colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
				colliderModel->SetMaterial(i, colliderMat);

			auto& gfxComponent = m_chunkEntities[chunkId].get_or_emplace<Nz::GraphicsComponent>();
			gfxComponent.AttachRenderable(std::move(colliderModel), 0x0000FFFF);
	}
#endif
	}
}
