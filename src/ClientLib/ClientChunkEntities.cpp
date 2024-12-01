// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/IndexBuffer.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/VertexBuffer.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/BufferPropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/OptionValuePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <fmt/format.h>
#include <numeric>

namespace tsom
{
	namespace
	{
		struct ClientChunkDataComponent
		{
			std::shared_ptr<Nz::MaterialInstance> material;
			std::shared_ptr<Nz::Model> model;
		};
	}

	ClientChunkEntities::ClientChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const ClientBlockLibrary& blockLibrary, std::size_t layerIndex) :
	ChunkEntities(app, world, chunkContainer, blockLibrary, layerIndex, NoInit{}),
	m_isCollisionGenerationEnabled(true),
	m_shouldDrawDebugColliders(false)
	{
		// FIXME: A lot of data can be shared between client chunk instances
		auto& filesystem = app.GetComponent<Nz::FilesystemAppComponent>();

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
		settings.AddValueProperty<float>("AlphaTestThreshold", 0.5f);
		settings.AddValueProperty<float>("ShadowMapNormalOffset", 0.f);
		settings.AddValueProperty<float>("ShadowPosScale", 1.f - 0.0025f);
		settings.AddTextureProperty("BaseColorMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("AlphaMap", Nz::ImageType::E2D_Array);
		settings.AddTextureProperty("DetailMap", Nz::ImageType::E2D_Array);
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
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("DetailMap", "HasDetailTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("EmissiveMap", "HasEmissiveTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("HeightMap", "HasHeightTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("MetallicMap", "HasMetallicTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("NormalMap", "HasNormalTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("RoughnessMap", "HasRoughnessTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("SpecularMap", "HasSpecularTexture"));

		settings.AddValueProperty<float>("ChunkBlockSize", 1.f);
		settings.AddValueProperty<Nz::Vector3f>("ChunkSize", 32.f); // TODO: Put as Vector3ui and cast in property handler
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("ChunkBlockSize"));
		settings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("ChunkSize"));
		settings.AddBufferProperty("VoxelData");
		settings.AddPropertyHandler(std::make_unique<Nz::BufferPropertyHandler>("VoxelData"));

		Nz::MaterialPass forwardPass;
		forwardPass.states.depthBuffer = true;
		forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
		forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.ChunkPBR"));
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

		auto chunkMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.ChunkPBR");

		m_chunkReferenceMaterial = chunkMaterial->Instantiate();
		m_chunkReferenceMaterial->SetTextureProperty("BaseColorMap", blockLibrary.GetBaseColorTexture(), blockSampler);
		m_chunkReferenceMaterial->SetTextureProperty("NormalMap", blockLibrary.GetNormalTexture(), blockSampler);
		m_chunkReferenceMaterial->SetTextureProperty("DetailMap", blockLibrary.GetDetailTexture(), blockSampler);
		m_chunkReferenceMaterial->SetValueProperty("ShadowPosScale", 1.f);
		m_chunkReferenceMaterial->SetValueProperty("AlphaTest", true);
		m_chunkReferenceMaterial->UpdatePassesStates({ "ShadowPass", "DistanceShadowPass" }, [](Nz::RenderStates& states)
		{
			states.frontFace = Nz::FrontFace::CounterClockwise;
			states.depthBias = true;
			states.depthBiasConstantFactor = 2.f;
			states.depthBiasSlopeFactor = 2.5f;
			return true;
		});

		if (blockLibrary.GetLayerData(layerIndex).isBlended)
			m_chunkReferenceMaterial->ApplyPreset(Nz::MaterialInstancePreset::AdditiveBlended);

		// Create worst case index buffer (alternating checkered voxels)
		constexpr Nz::UInt32 WorstFaceCount = 6 * (Constants::MaxChunkSize / 2) * (Constants::MaxChunkSize / 2) * Constants::MaxChunkSize;
		constexpr Nz::UInt32 WorstIndexCount = 6 * WorstFaceCount;

		std::vector<Nz::UInt32> indices(WorstIndexCount);
		Nz::UInt32* indexPtr = indices.data();
		for (std::size_t i = 0; i < WorstFaceCount; ++i)
		{
			*indexPtr++ = i * 4 + 0;
			*indexPtr++ = i * 4 + 2;
			*indexPtr++ = i * 4 + 1;

			*indexPtr++ = i * 4 + 1;
			*indexPtr++ = i * 4 + 2;
			*indexPtr++ = i * 4 + 3;
		}

		auto& renderDevice = Nz::Graphics::Instance()->GetRenderDevice();
		std::shared_ptr<Nz::RenderBuffer> worstIndexBuffer = renderDevice->InstantiateBuffer(Nz::BufferType::Index, WorstIndexCount * sizeof(Nz::UInt32), Nz::BufferUsage::DeviceLocal | Nz::BufferUsage::Read, indices.data());

		m_chunkGraphicalMesh = std::make_shared<Nz::GraphicalMesh>();
		m_chunkGraphicalMesh->AddSubMesh({
			.indexBuffer = std::move(worstIndexBuffer),
			.indexType = Nz::IndexType::U32,
			.indexCount = WorstIndexCount
		});

		// TODO: Configure with chunk settings
		m_chunkGraphicalMesh->UpdateAABB(Nz::Boxf(-16.f, -16.f, -16.f, 32.f, 32.f, 32.f));

		FillChunks();
	}

	auto ClientChunkEntities::BuildMeshData(const Chunk& chunk) -> VoxelBuffer
	{
		nzsl::FieldOffsets faceDataOffsets(nzsl::StructLayout::Std430);
		std::size_t data1Offset = faceDataOffsets.AddField(nzsl::StructFieldType::UInt1);
		std::size_t textureSliceOffset = faceDataOffsets.AddField(nzsl::StructFieldType::Float1);

		std::size_t alignedSize = faceDataOffsets.GetAlignedSize();

		std::vector<std::uint8_t> faceData;
		std::size_t faceCount = 0;

		Nz::Vector3f chunkOffset = chunk.GetContainer().GetChunkOffset(chunk.GetIndices());
		Nz::Vector3f chunkHalfSize = Nz::Vector3f(chunk.GetSize()) * 0.5f;

		auto AddFaces = [&](BlockIndex blockContent, const Nz::Vector3ui& blockIndices, Direction direction)
		{
			const auto& blockData = m_blockLibrary.GetBlockData(blockContent);

			assert(blockIndices.x <= 30 && blockIndices.y <= 30 && blockIndices.z <= 30);
			Nz::Vector3f blockOffset = (Nz::Vector3f(blockIndices.x, blockIndices.z, blockIndices.y) - chunkHalfSize + Nz::Vector3f(0.5f)) * chunk.GetBlockSize();

			Nz::Vector3f blockCenter = chunkOffset + blockOffset;

			/*Nz::Vector3f faceCenter = std::accumulate(faceCorners.begin(), faceCorners.end(), Nz::Vector3f::Zero()) / faceCorners.size();

			Nz::Vector3f edgeCenter = (faceCorners[0] + faceCorners[1]) * 0.5f;
			Nz::Vector3f tangent = Nz::Vector3f::Normalize(edgeCenter - faceCenter);*/

			Direction faceDirection = DirectionFromNormal(Nz::Vector3f::Normalize(blockCenter));
			Nz::Vector3f faceUp = s_dirNormals[faceDirection];

			// Make up the rotation from the face up to the regular up
			Nz::Quaternionf upRotation = Nz::Quaternionf::RotationBetween(faceUp, Nz::Vector3f::Up());

			// Compute texture direction based on face direction in regular orientation
			Direction texDirection = DirectionFromNormal(upRotation * s_dirNormals[direction]);
			std::size_t textureIndex = blockData.texIndices[texDirection];

			faceData.resize(faceData.size() + alignedSize);
			void* faceBaseAddr = &faceData[faceCount * alignedSize];
			Nz::AccessByOffset<Nz::UInt32&>(faceBaseAddr, data1Offset) = (static_cast<Nz::UInt32>(faceDirection) << 18) | (static_cast<Nz::UInt32>(direction) << 15) | (blockIndices.z << 10) | (blockIndices.y << 5) | blockIndices.x;
			Nz::AccessByOffset<float&>(faceBaseAddr, textureSliceOffset) = textureIndex;

			faceCount++;
		};

		chunk.BuildFaces(m_layerIndex, AddFaces, true, true);

		return { faceCount, std::move(faceData) };
	}

	auto ClientChunkEntities::ProcessChunkUpdate(const Chunk& chunk, DirectionMask neighborMask) -> ColliderModelUpdateJob*
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
			const Chunk* chunk = m_chunkContainer.GetChunk(chunkIndices);
			assert(chunk);

			ColliderModelUpdateJob&& colliderModelUpdateJob = static_cast<ColliderModelUpdateJob&&>(job);

			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);

			if (m_isCollisionGenerationEnabled)
			{
				auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
				rigidBody.SetCollider(std::move(colliderModelUpdateJob.collider), false);
			}

			if (colliderModelUpdateJob.voxelData.faceCount == 0 && !m_shouldDrawDebugColliders)
				return; // chunk has no rendering

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
			{
				ClientChunkDataComponent* clientChunkData = visualEntity.try_get<ClientChunkDataComponent>();
				if (colliderModelUpdateJob.voxelData.faceCount > 0)
				{
					// TODO: Pool storage buffers (in a smart way to avoid memory cost)
					if (!clientChunkData)
					{
						clientChunkData = &visualEntity.emplace<ClientChunkDataComponent>();
						clientChunkData->material = m_chunkReferenceMaterial->Clone();
						clientChunkData->model = std::make_shared<Nz::Model>(m_chunkGraphicalMesh);
						clientChunkData->model->SetMaterial(0, clientChunkData->material);
						clientChunkData->model->UpdateRenderLayer(m_blockLibrary.GetLayerData(m_layerIndex).renderLayer);

						clientChunkData->material->SetValueProperty("ChunkBlockSize", chunk->GetBlockSize());
						clientChunkData->material->SetValueProperty("ChunkSize", Nz::Vector3f(chunk->GetSize()));

						gfxComponent.AttachRenderable(clientChunkData->model, tsom::Constants::RenderMask3D);
					}

					// Create storage buffer
					// TODO: Move GPU upload to async task (should almost already work on Vulkan, problem is OpenGL)

					auto& renderDevice = Nz::Graphics::Instance()->GetRenderDevice();

					std::shared_ptr<Nz::RenderBuffer> chunkStorage = renderDevice->InstantiateBuffer(Nz::BufferType::Storage, colliderModelUpdateJob.voxelData.bufferData.size(), Nz::BufferUsage::DeviceLocal | Nz::BufferUsage::Read, colliderModelUpdateJob.voxelData.bufferData.data());
					clientChunkData->material->SetBufferProperty("VoxelData", chunkStorage);
					clientChunkData->model->SetIndexCount(0, colliderModelUpdateJob.voxelData.faceCount * 6);
				}
				else
				{
					if (clientChunkData)
					{
						gfxComponent.DetachRenderable(clientChunkData->model);
						visualEntity.erase<ClientChunkDataComponent>();
					}
				}
			}

			if (m_shouldDrawDebugColliders)
				UpdateChunkDebugCollider(chunkIndices);
		};

		auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
		if (m_isCollisionGenerationEnabled)
		{
			taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
			{
				if (updateJob->cancelled)
					return;

				chunkPtr->LockRead();
				//Nz::HighPrecisionClock c;
				updateJob->collider = chunkPtr->BuildCollider(m_layerIndex);
				//fmt::print("chunk physics update took {}us\n", c.GetElapsedTime().AsMicroseconds());
				chunkPtr->UnlockRead();

				updateJob->jobDone++;
			});
		}

		taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
		{
			if (updateJob->cancelled)
				return;

			chunkPtr->LockRead();
			//Nz::HighPrecisionClock c;
			updateJob->voxelData = BuildMeshData(*chunkPtr);
			//fmt::print("chunk graphics update took {}us\n", c.GetElapsedTime().AsMicroseconds());
			chunkPtr->UnlockRead();

			updateJob->jobDone++;
		});

		// Add neighbor chunks
		for (Direction neighborDir : neighborMask)
		{
			ChunkIndices neighborIndices = chunk.GetIndices() + s_chunkDirOffset[neighborDir];
			const Chunk* neighborChunk = m_chunkContainer.GetChunk(neighborIndices);
			if (!neighborChunk || !neighborChunk->HasContent() || !neighborChunk->IsLayerRegistered(m_layerIndex))
				continue;

			updateJob->chunkDependencies.push_back(neighborIndices);

			// Trigger our neighbor update
			if (!m_updateJobs.contains(neighborIndices))
				ProcessChunkUpdate(*neighborChunk, 0);
		}

		ColliderModelUpdateJob* jobPtr = updateJob.get();
		m_updateJobs.insert_or_assign(chunk.GetIndices(), std::move(updateJob));

		return jobPtr;
	}

	void ClientChunkEntities::UpdateChunkDebugCollider(const ChunkIndices& chunkIndices)
	{
		entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);

		// Remember the chunkEntity is the logical one, so it has no GraphicsComponent when not debugging
		auto& gfxComponent = chunkEntity.get_or_emplace<Nz::GraphicsComponent>();
		gfxComponent.Clear();

		auto& rigidBodyComponent = chunkEntity.get<Nz::RigidBody3DComponent>();
		const std::shared_ptr<Nz::Collider3D>& collider = rigidBodyComponent.GetCollider();
		if (collider->GetType() == Nz::ColliderType3D::Empty)
			return;

		std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
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

		std::shared_ptr<Nz::Model> colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
		for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
			colliderModel->SetMaterial(i, colliderMat);

		gfxComponent.AttachRenderable(colliderModel, tsom::Constants::RenderMask3D);
	}
}
