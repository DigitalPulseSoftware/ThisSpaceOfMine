// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/ClientChunkEntities.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/AppFilesystemComponent.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PredefinedMaterials.hpp>
#include <Nazara/Graphics/PropertyHandler/OptionValuePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
#include <Nazara/Utility/IndexBuffer.hpp>
#include <Nazara/Utility/VertexBuffer.hpp>

namespace tsom
{
	ClientChunkEntities::ClientChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary) :
	ChunkEntities(world, chunkContainer, blockLibrary, NoInit{})
	{
		auto& filesystem = app.GetComponent<Nz::AppFilesystemComponent>();

		Nz::TextureSamplerInfo blockSampler;
		blockSampler.anisotropyLevel = 16;
		blockSampler.magFilter = Nz::SamplerFilter::Linear;
		blockSampler.minFilter = Nz::SamplerFilter::Linear;
		blockSampler.wrapModeU = Nz::SamplerWrap::Repeat;
		blockSampler.wrapModeV = Nz::SamplerWrap::Repeat;

		auto& materialPassRegistry = Nz::Graphics::Instance()->GetMaterialPassRegistry();
		std::size_t depthPassIndex = materialPassRegistry.GetPassIndex("DepthPass");
		std::size_t shadowPassIndex = materialPassRegistry.GetPassIndex("ShadowPass");
		std::size_t distanceShadowPassIndex = materialPassRegistry.GetPassIndex("DistanceShadowPass");
		std::size_t forwardPassIndex = materialPassRegistry.GetPassIndex("ForwardPass");

		Nz::MaterialSettings settings;
		settings.AddValueProperty<Nz::Color>("BaseColor", Nz::Color::White());
		settings.AddValueProperty<bool>("AlphaTest", false);
		settings.AddValueProperty<float>("AlphaTestThreshold", 0.2f);
		settings.AddValueProperty<float>("ShadowMapNormalOffset", 0.f);
		settings.AddValueProperty<float>("ShadowPosScale", 1.f - 0.0025f);
		settings.AddTextureProperty("BaseColorMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("AlphaMap", Nz::ImageType::E2D_Array);
		settings.AddPropertyHandler(std::make_unique<Nz::OptionValuePropertyHandler>("AlphaTest", "AlphaTest"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("BaseColorMap", "HasBaseColorTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("AlphaMap", "HasAlphaTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("BaseColor"));
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("AlphaTestThreshold"));
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("ShadowMapNormalOffset"));
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("ShadowPosScale"));
		settings.AddTextureProperty("EmissiveMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("HeightMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("MetallicMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("NormalMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("RoughnessMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("SpecularMap", Nz::ImageType::E2D_Array);
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("EmissiveMap", "HasEmissiveTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("HeightMap", "HasHeightTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("MetallicMap", "HasMetallicTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("NormalMap", "HasNormalTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("RoughnessMap", "HasRoughnessTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("SpecularMap", "HasSpecularTexture"));

		Nz::MaterialPass forwardPass;
		forwardPass.states.depthBuffer = true;
		forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.BlockPBR"));
		settings.AddPass(forwardPassIndex, forwardPass);

		Nz::MaterialPass depthPass = forwardPass;
		depthPass.options[Nz::CRC32("DepthPass")] = true;
		settings.AddPass(depthPassIndex, depthPass);

		Nz::MaterialPass shadowPass = depthPass;
		shadowPass.options[Nz::CRC32("ShadowPass")] = true;
		shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
		shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetRenderDevice()->GetEnabledFeatures().depthClamping;
		settings.AddPass(shadowPassIndex, shadowPass);

		Nz::MaterialPass distanceShadowPass = shadowPass;
		distanceShadowPass.options[Nz::CRC32("DistanceDepth")] = true;
		settings.AddPass(distanceShadowPassIndex, distanceShadowPass);

		auto chunkMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.BlockPBR");

		m_chunkMaterial = chunkMaterial->Instantiate();
		m_chunkMaterial->SetTextureProperty("BaseColorMap", blockLibrary.GetBaseColorTexture(), blockSampler);
		m_chunkMaterial->SetTextureProperty("NormalMap", blockLibrary.GetNormalTexture(), blockSampler);
		m_chunkMaterial->SetValueProperty("ShadowPosScale", 1.f);
		m_chunkMaterial->UpdatePassesStates({ "ShadowPass", "DistanceShadowPass" }, [](Nz::RenderStates& states)
		{
			states.frontFace = Nz::FrontFace::CounterClockwise;
			states.depthBias = true;
			states.depthBiasConstantFactor = 2.f;
			states.depthBiasSlopeFactor = 2.5f;
			return true;
		});

		// VertexDeclaration
		auto NewDeclaration = [](Nz::VertexInputRate inputRate, std::initializer_list<Nz::VertexDeclaration::ComponentEntry> components)
		{
			return std::make_shared<Nz::VertexDeclaration>(inputRate, std::move(components));
		};

		m_chunkVertexDeclaration = NewDeclaration(Nz::VertexInputRate::Vertex, {
			{
				Nz::VertexComponent::Position,
				Nz::ComponentType::Float3,
				0
			},
			{
				Nz::VertexComponent::Normal,
				Nz::ComponentType::Float3,
				0
			},
			{
				Nz::VertexComponent::TexCoord,
				Nz::ComponentType::Float3,
				0
			},
			{
				Nz::VertexComponent::Tangent,
				Nz::ComponentType::Float3,
				0
			}
		});

		FillChunks();
	}

	std::shared_ptr<Nz::Model> ClientChunkEntities::BuildModel(const Chunk* chunk)
	{
		std::vector<Nz::UInt32> indices;
		std::vector<VertexStruct> vertices;

		auto AddVertices = [&](Nz::UInt32 count)
		{
			Chunk::VertexAttributes vertexAttributes;

			vertexAttributes.firstIndex = Nz::SafeCast<Nz::UInt32>(vertices.size());
			vertices.resize(vertices.size() + count);
			vertexAttributes.position = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].position, sizeof(vertices.front()));
			vertexAttributes.normal = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].normal, sizeof(vertices.front()));
			vertexAttributes.tangent = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].tangent, sizeof(vertices.front()));
			vertexAttributes.uv = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].uvw, sizeof(vertices.front()));

			return vertexAttributes;
		};

		chunk->BuildMesh(m_blockLibrary, indices, m_chunkContainer.GetCenter() - m_chunkContainer.GetChunkOffset(chunk->GetIndices()), AddVertices);
		if (indices.empty())
			return nullptr;

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, Nz::SafeCast<Nz::UInt32>(indices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(m_chunkVertexDeclaration, Nz::SafeCast<Nz::UInt32>(vertices.size()), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();
		staticMesh->GenerateTangents();

		Nz::Mesh chunkMesh;
		chunkMesh.CreateStatic();
		chunkMesh.AddSubMesh(std::move(staticMesh));

		std::shared_ptr<Nz::GraphicalMesh> gfxMesh = Nz::GraphicalMesh::BuildFromMesh(chunkMesh);

		std::shared_ptr<Nz::Model> model = std::make_shared<Nz::Model>(std::move(gfxMesh));
		model->SetMaterial(0, m_chunkMaterial);

		return model;
	}

	void ClientChunkEntities::CreateChunkEntity(std::size_t chunkId, const Chunk* chunk)
	{
		ChunkEntities::CreateChunkEntity(chunkId, chunk);

		std::shared_ptr<Nz::Model> model = BuildModel(chunk);

		auto& gfxComponent = m_chunkEntities[chunkId].emplace<Nz::GraphicsComponent>();
		if (model)
			gfxComponent.AttachRenderable(std::move(model), 0x0000FFFF);

		UpdateChunkDebugCollider(chunkId);
	}

	void ClientChunkEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		ChunkEntities::UpdateChunkEntity(chunkId);

		std::shared_ptr<Nz::Model> model = BuildModel(m_chunkContainer.GetChunk(chunkId));

		auto& gfxComponent = m_chunkEntities[chunkId].get_or_emplace<Nz::GraphicsComponent>();
		gfxComponent.Clear();
		if (model)
			gfxComponent.AttachRenderable(std::move(model), 0x0000FFFF);

		UpdateChunkDebugCollider(chunkId);
	}

	void ClientChunkEntities::UpdateChunkDebugCollider(std::size_t chunkId)
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