// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientConfigs.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/ConfigFile.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/IndexBuffer.hpp>
#include <Nazara/Core/SpatialSort.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/VertexBuffer.hpp>
#include <Nazara/Core/VertexMapper.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/BufferPropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/OptionValuePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	ClientChunkEntities::ClientChunkEntities(Nz::ApplicationBase& app, ConfigFile& config, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary, std::size_t layerIndex) :
	ChunkEntities(app, world, chunkContainer, blockLibrary, layerIndex, NoInit{}),
	m_configFile(config),
	m_isCollisionGenerationEnabled(true)
	{
		auto& clientAssets = app.GetComponent<ClientAssetLibraryAppComponent>();

		std::shared_ptr<Nz::Material> blockMaterial = clientAssets.QueryMaterial("Chunk");
		if (!blockMaterial)
		{
			auto& materialPassRegistry = Nz::Graphics::Instance()->GetMaterialPassRegistry();
			std::size_t depthPassIndex = materialPassRegistry.GetPassIndex("DepthPass");
			std::size_t shadowPassIndex = materialPassRegistry.GetPassIndex("ShadowPass");
			std::size_t distanceShadowPassIndex = materialPassRegistry.GetPassIndex("DistanceShadowPass");
			std::size_t forwardPassIndex = materialPassRegistry.GetPassIndex("ForwardPass");

			Nz::MaterialSettings settings;
			settings.AddValueProperty<Nz::Color>("BaseColor", Nz::Color::White());
			settings.AddValueProperty<bool>("AlphaTest", false);
			settings.AddValueProperty<float>("AlphaTestThreshold", 0.6f);
			settings.AddValueProperty<float>("ShadowMapNormalOffset", 0.f);
			settings.AddValueProperty<float>("ShadowPosScale", 1.f - 0.0025f);
			settings.AddValueProperty<Nz::Vector3f>("TriplanarOffset", Nz::Vector3f::Zero());
			settings.AddBufferProperty("GlobalBlockData");
			settings.AddTextureProperty("BlockTexture1", Nz::ImageType::E2D_Array);
			settings.AddTextureProperty("BlockTexture2", Nz::ImageType::E2D_Array);
			settings.AddTextureProperty("BlockTexture3", Nz::ImageType::E2D_Array);
			settings.AddTextureProperty("BlockTexture4", Nz::ImageType::E2D_Array);

			settings.AddPropertyHandler<Nz::BufferPropertyHandler>("GlobalBlockData");
			settings.AddPropertyHandler<Nz::OptionValuePropertyHandler>("AlphaTest");
			settings.AddPropertyHandler<Nz::TexturePropertyHandler>("BlockTexture1");
			settings.AddPropertyHandler<Nz::TexturePropertyHandler>("BlockTexture2");
			settings.AddPropertyHandler<Nz::TexturePropertyHandler>("BlockTexture3");
			settings.AddPropertyHandler<Nz::TexturePropertyHandler>("BlockTexture4");
			settings.AddPropertyHandler<Nz::UniformValuePropertyHandler>("BaseColor");
			settings.AddPropertyHandler<Nz::UniformValuePropertyHandler>("AlphaTestThreshold");
			settings.AddPropertyHandler<Nz::UniformValuePropertyHandler>("ShadowMapNormalOffset");
			settings.AddPropertyHandler<Nz::UniformValuePropertyHandler>("ShadowPosScale");

			Nz::MaterialPass forwardPass;
			forwardPass.states.depthBuffer = true;
			forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
			forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.BlockPBR"));
			settings.AddPass(forwardPassIndex, forwardPass);

			Nz::MaterialPass depthPass = forwardPass;
			depthPass.options[nzsl::Ast::HashOption("DepthPass")] = true;
			settings.AddPass(depthPassIndex, depthPass);

			Nz::MaterialPass shadowPass = depthPass;
			shadowPass.options[nzsl::Ast::HashOption("ShadowPass")] = true;
			shadowPass.states.depthCompare = Nz::RendererComparison::LessOrEqual; //< TODO: Reverse depth for shadow pass?
			shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
			shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetRenderDevice()->GetEnabledFeatures().depthClamping;
			settings.AddPass(shadowPassIndex, shadowPass);

			Nz::MaterialPass distanceShadowPass = shadowPass;
			distanceShadowPass.options[nzsl::Ast::HashOption("DistanceDepth")] = true;
			settings.AddPass(distanceShadowPassIndex, distanceShadowPass);

			blockMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.BlockPBR");
			clientAssets.RegisterMaterial("Chunk", blockMaterial);
		}

		Nz::TextureSamplerInfo blockSampler;
		blockSampler.anisotropyLevel = 16;
		blockSampler.magFilter = Nz::SamplerFilter::Linear;
		blockSampler.minFilter = Nz::SamplerFilter::Linear;
		blockSampler.wrapModeU = Nz::SamplerWrap::Repeat;
		blockSampler.wrapModeV = Nz::SamplerWrap::Repeat;

		m_chunkMaterial = blockMaterial->Instantiate();
		m_chunkMaterial->SetBufferProperty("GlobalBlockData", blockLibrary.GetGlobalBlockBuffer());
		m_chunkMaterial->SetTextureProperty("BlockTexture1", blockLibrary.GetBlockTexture(ClientAssetCookRegistry::TextureType::BC1), blockSampler);
		m_chunkMaterial->SetTextureProperty("BlockTexture2", blockLibrary.GetBlockTexture(ClientAssetCookRegistry::TextureType::BC3), blockSampler);
		m_chunkMaterial->SetTextureProperty("BlockTexture3", blockLibrary.GetBlockTexture(ClientAssetCookRegistry::TextureType::BC4), blockSampler);
		m_chunkMaterial->SetTextureProperty("BlockTexture4", blockLibrary.GetBlockTexture(ClientAssetCookRegistry::TextureType::BC5), blockSampler);
		m_chunkMaterial->SetValueProperty("ShadowPosScale", 1.f);
		m_chunkMaterial->SetValueProperty("AlphaTest", true);
		m_chunkMaterial->UpdatePassesStates({ "ShadowPass", "DistanceShadowPass" }, [](Nz::RenderStates& states)
		{
			states.frontFace = Nz::FrontFace::CounterClockwise;
			states.depthBias = true;
			states.depthBiasConstantFactor = 2.f;
			states.depthBiasSlopeFactor = 2.5f;
			return true;
		});

		if (blockLibrary.GetLayerData(layerIndex).isBlended)
			m_chunkMaterial->ApplyPreset(Nz::MaterialInstancePreset::AdditiveBlended);

		// VertexDeclaration
		auto NewDeclaration = [](Nz::VertexInputRate inputRate, std::initializer_list<Nz::VertexDeclaration::ComponentEntry> components)
		{
			return std::make_shared<Nz::VertexDeclaration>(inputRate, components);
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
				Nz::VertexComponent::Userdata,
				Nz::ComponentType::UInt1,
				0
			}
		});

		m_onVisualChunkNormalSmoothAngleUpdatedSlot.Connect(m_configFile.GetFloatUpdateSignal(Config::Visual_ChunkNormalSmoothAngle), [this](double /*newValue*/)
		{
			RebuildAllChunks();
		});

		FillChunks();
	}

	std::shared_ptr<Nz::Mesh> ClientChunkEntities::BuildMesh(const Chunk& chunk)
	{
		std::vector<Nz::UInt32> indices;
		std::vector<VertexStruct> vertices;

		auto AddVertices = [&](const Nz::Vector3ui& /*blockIndices*/, Direction /*direction*/)
		{
			Chunk::VertexAttributes vertexAttributes;

			vertexAttributes.firstIndex = Nz::SafeCast<Nz::UInt32>(vertices.size());
			vertices.resize(vertices.size() + 4);
			vertexAttributes.position = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].position, sizeof(vertices.front()));
			vertexAttributes.normal = Nz::SparsePtr<Nz::Vector3f>(&vertices[vertexAttributes.firstIndex].normal, sizeof(vertices.front()));
			vertexAttributes.blockIndex = Nz::SparsePtr<Nz::UInt32>(&vertices[vertexAttributes.firstIndex].blockIndex, sizeof(vertices.front()));

			return vertexAttributes;
		};

		chunk.BuildMesh(m_layerIndex, indices, m_chunkContainer.GetCenter() - m_chunkContainer.GetChunkOffset(chunk.GetIndices()), AddVertices);
		if (indices.empty())
			return nullptr;

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, Nz::SafeCast<Nz::UInt32>(indices.size()), Nz::BufferUsage::IndexBuffer, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(m_chunkVertexDeclaration, Nz::SafeCast<Nz::UInt32>(vertices.size()), Nz::BufferUsage::VertexBuffer, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();

		Nz::DegreeAnglef smoothLimitAngle = m_configFile.GetFloatValue<float>(Config::Visual_ChunkNormalSmoothAngle);
		if (smoothLimitAngle > 0.0f)
		{
			Nz::VertexMapper mapper(*staticMesh);
			Nz::UInt32 vertexCount = mapper.GetVertexCount();

			Nz::SparsePtr<Nz::Vector3f> normals = mapper.GetComponentPtr<Nz::Vector3f>(Nz::VertexComponent::Normal);
			Nz::SparsePtr<Nz::Vector3f> positions = mapper.GetComponentPtr<Nz::Vector3f>(Nz::VertexComponent::Position);

			std::vector<Nz::Vector3f> newNormals(vertexCount);

			Nz::SpatialSort spatialSort;
			spatialSort.Append(positions, vertexCount);

			std::vector<Nz::UInt32> sortResult;

			float fLimit = smoothLimitAngle.GetCos();
			for (Nz::UInt32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				sortResult.clear();
				spatialSort.FindPositions(positions[vertexIndex], 0.01f, sortResult);

				Nz::Vector3f vertexNormal = normals[vertexIndex];

				Nz::Vector3f normalSum = vertexNormal;
				for (Nz::UInt32 resultIndex : sortResult)
				{
					if (vertexIndex == resultIndex)
						continue;

					Nz::Vector3f normal = normals[resultIndex];
					if (Nz::Vector3f::DotProduct(normal, vertexNormal) >= fLimit)
						normalSum += normal;
				}

				newNormals[vertexIndex] = normalSum.Normalize();
			}

			for (Nz::UInt32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
				normals[vertexIndex] = newNormals[vertexIndex];
		}

		std::shared_ptr<Nz::Mesh> chunkMesh = std::make_shared<Nz::Mesh>();
		chunkMesh->CreateStatic();
		chunkMesh->AddSubMesh(std::move(staticMesh));

		return chunkMesh;
	}

	auto ClientChunkEntities::ProcessChunkUpdate(const Chunk& chunk, NeighborChunkMask neighborMask) -> ColliderModelUpdateJob*
	{
		NazaraAssert(chunk.HasContent());

		// Try to cancel current update job to void useless work
		if (auto it = m_updateJobs.find(chunk.GetIndices()); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;
		}

		std::shared_ptr<ColliderModelUpdateJob> updateJob = std::make_shared<ColliderModelUpdateJob>();
		updateJob->taskCount = (m_isCollisionGenerationEnabled) ? 2 : 1;

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, UpdateJob&& job)
		{
			ColliderModelUpdateJob&& colliderUpdateJob = static_cast<ColliderModelUpdateJob&&>(job);

			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);

			if (m_isCollisionGenerationEnabled)
			{
				auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
				rigidBody.SetCollider(std::move(colliderUpdateJob.collider), false);
			}

			entt::handle visualEntity;
			if (VisualEntityComponent* visualEntityComponent = chunkEntity.try_get<VisualEntityComponent>())
				visualEntity = visualEntityComponent->visualEntity;
			else
			{
				// First time, create visual entity

				// Get root visual entity
				auto& visualRootEntity = m_parentEntity.get<VisualEntityComponent>();

				visualEntity = m_world.CreateEntity();

				auto& visualNode = visualEntity.emplace<Nz::NodeComponent>();
				visualNode.CopyLocalTransform(chunkEntity.get<Nz::NodeComponent>());
				visualNode.SetParent(visualRootEntity.visualEntity);

				// Register our visual entity
				auto& chunkVisualEntity = chunkEntity.emplace<VisualEntityComponent>();
				chunkVisualEntity.visualEntity = visualEntity;

				auto& entityOwnerComp = chunkEntity.get_or_emplace<EntityOwnerComponent>();
				entityOwnerComp.Register(visualEntity);
			}

			auto& gfxComponent = visualEntity.get_or_emplace<Nz::GraphicsComponent>();
			gfxComponent.Clear();

			if (colliderUpdateJob.mesh)
			{
				// TODO: Move GPU upload to async task (should almost already work on Vulkan, problem is OpenGL)
				std::shared_ptr<Nz::GraphicalMesh> gfxMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderUpdateJob.mesh);

				std::shared_ptr<Nz::Model> model = std::make_shared<Nz::Model>(std::move(gfxMesh));
				model->SetMaterial(0, m_chunkMaterial);
				model->UpdateRenderLayer(m_blockLibrary.GetLayerData(m_layerIndex).renderLayer);

				gfxComponent.AttachRenderable(std::move(model), tsom::Constants::RenderMask3D);
			}

			UpdateChunkDebugCollider(chunkIndices);
		};

		auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
		if (m_isCollisionGenerationEnabled)
		{
			taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
			{
				if (updateJob->cancelled)
					return;

				ChunkReadLock lock(chunkPtr.get());
				updateJob->collider = chunkPtr->BuildCollider(m_layerIndex);
				updateJob->jobDone++;
			});
		}

		taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
		{
			if (updateJob->cancelled)
				return;

			// FIXME: If ClientChunkEntities is deleted before job finished, it can result in a crash
			ChunkReadLock lock(chunkPtr.get());
			updateJob->mesh = BuildMesh(*chunkPtr);
			updateJob->jobDone++;
		});

		// Add neighbor chunks
		for (NeighborChunk neighborChunk : neighborMask)
		{
			ChunkIndices neighborIndices = chunk.GetIndices() + s_neighborChunkOffset[neighborChunk];
			const Chunk* neighborChunkPtr = m_chunkContainer.GetChunk(neighborIndices);
			if (!neighborChunkPtr || !neighborChunkPtr->HasContent() || !neighborChunkPtr->IsLayerRegistered(m_layerIndex))
				continue;

			updateJob->chunkDependencies.push_back(neighborIndices);

			// Trigger our neighbor update
			if (!m_updateJobs.contains(neighborIndices))
				ProcessChunkUpdate(*neighborChunkPtr, 0);
		}

		ColliderModelUpdateJob* jobPtr = updateJob.get();
		m_updateJobs.insert_or_assign(chunk.GetIndices(), std::move(updateJob));

		return jobPtr;
	}

	void ClientChunkEntities::UpdateChunkDebugCollider(const ChunkIndices& chunkIndices)
	{
#if 0
		std::shared_ptr<Nz::Model> colliderModel;
		{
			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);

			auto& rigidBodyComponent = chunkEntity.get<Nz::RigidBody3DComponent>();
			const std::shared_ptr<Nz::Collider3D>& collider = rigidBodyComponent.GetCollider();
			if (collider->GetType() == Nz::ColliderType3D::Empty)
				return;

			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			colliderMat->ApplyPreset(Nz::MaterialInstancePreset::ReverseZ);
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::StaticMesh> colliderSubmesh = collider->GenerateDebugMesh();
			if (!colliderSubmesh)
				return;

			std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(std::move(colliderSubmesh));
			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

			colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
				colliderModel->SetMaterial(i, colliderMat);

			auto& gfxComponent = chunkEntity.get_or_emplace<Nz::GraphicsComponent>();
			gfxComponent.AttachRenderable(std::move(colliderModel), tsom::Constants::RenderMask3D);
	}
#endif
	}
}
