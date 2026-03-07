// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Rendering/AtmosphereScatteringPipelinePass.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/AbstractViewer.hpp>
#include <Nazara/Graphics/FrameGraph.hpp>
#include <Nazara/Graphics/FramePipelinePassRegistry.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderBufferPool.hpp>
#include <Nazara/Graphics/ViewerInstance.hpp>
#include <NZSL/Math/FieldOffsets.hpp>

namespace tsom
{
	namespace
	{
		struct AtmosphereScatteringData
		{
			nzsl::FieldOffsets fieldOffsets;

			std::size_t sunDirOffset;
			std::size_t sunIntensityOffset;
			std::size_t planetPositionOffset;
			std::size_t planetDimensionsOffset;
			std::size_t planetCornerRadiusOffset;
			std::size_t atmosphereMaxHeightOffset;
			std::size_t rayleighBetaOffset;
			std::size_t mieBetaOffset;
			std::size_t ambientBetaOffset;
			std::size_t absorptionBetaOffset;
			std::size_t mieScatteringDirectionOffset;
			std::size_t rayleighHeightOffset;
			std::size_t mieHeightOffset;
			std::size_t heightAbsorptionOffset;
			std::size_t absorptionFalloffOffset;
			std::size_t primaryStepsOffset;
			std::size_t lightStepsOffset;

			std::size_t totalSize;

			static constexpr AtmosphereScatteringData Build()
			{
				AtmosphereScatteringData atmosphereScatteringData = { nzsl::FieldOffsets(nzsl::StructLayout::Std140) };
				atmosphereScatteringData.sunDirOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.sunIntensityOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.planetPositionOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.planetDimensionsOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.planetCornerRadiusOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.atmosphereMaxHeightOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.rayleighBetaOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.mieBetaOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.ambientBetaOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.absorptionBetaOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringData.mieScatteringDirectionOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.rayleighHeightOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.mieHeightOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.heightAbsorptionOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.absorptionFalloffOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
				atmosphereScatteringData.primaryStepsOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Int1);
				atmosphereScatteringData.lightStepsOffset = atmosphereScatteringData.fieldOffsets.AddField(nzsl::StructFieldType::Int1);

				atmosphereScatteringData.totalSize = atmosphereScatteringData.fieldOffsets.GetAlignedSize();

				return atmosphereScatteringData;
			}
		};

		constexpr AtmosphereScatteringData AtmosphereScatteringFields = AtmosphereScatteringData::Build();

		constexpr Nz::UInt32 MaxAtmosphereCount = 8;

		struct AtmosphereScatteringPassData
		{
			std::size_t invProjectionMatrixOffset;
			std::size_t invViewMatrixOffset;
			std::size_t viewerPositionOffset;
			std::size_t zNearOffset;

			std::size_t atmosphereCountOffset;
			std::size_t atmospheresOffset;

			std::size_t totalSize;

			static constexpr AtmosphereScatteringPassData Build()
			{
				nzsl::FieldOffsets fieldOffsets(nzsl::StructLayout::Std140);

				AtmosphereScatteringPassData atmosphereScatteringPassData;
				atmosphereScatteringPassData.invProjectionMatrixOffset = fieldOffsets.AddMatrix(nzsl::StructFieldType::Float1, 4, 4, true);
				atmosphereScatteringPassData.invViewMatrixOffset = fieldOffsets.AddMatrix(nzsl::StructFieldType::Float1, 4, 4, true);
				atmosphereScatteringPassData.viewerPositionOffset = fieldOffsets.AddField(nzsl::StructFieldType::Float3);
				atmosphereScatteringPassData.zNearOffset = fieldOffsets.AddField(nzsl::StructFieldType::Float1);

				atmosphereScatteringPassData.atmosphereCountOffset = fieldOffsets.AddField(nzsl::StructFieldType::UInt1);
				atmosphereScatteringPassData.atmospheresOffset = fieldOffsets.AddStructArray(AtmosphereScatteringFields.fieldOffsets, MaxAtmosphereCount);

				atmosphereScatteringPassData.totalSize = fieldOffsets.GetAlignedSize();

				return atmosphereScatteringPassData;
			}
		};

		constexpr AtmosphereScatteringPassData AtmosphereScatteringPassFields = AtmosphereScatteringPassData::Build();
	}

	AtmosphereScatteringPipelinePass::AtmosphereScatteringPipelinePass(PassData& passData, std::string passName, entt::registry& registry, const Nz::ParameterList& parameterList) :
	FramePipelinePass({}),
	m_passName(std::move(passName)),
	m_registry(registry),
	m_viewer(passData.viewer),
	m_shader(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.PlanetAtmosphere")
	{
		std::shared_ptr<Nz::RenderDevice> renderDevice = Nz::Graphics::Instance()->GetRenderDevice();

		m_passDataBufferPool = std::make_shared<Nz::RenderBufferPool>(renderDevice, Nz::BufferType::Uniform, AtmosphereScatteringPassFields.totalSize, 32);

		Nz::RenderPipelineLayoutInfo layoutInfo;
		layoutInfo.bindings.assign({
			{
				0, 0, 1,
				Nz::ShaderBindingType::UniformBuffer,
				nzsl::ShaderStageType::Fragment
			},
			{
				0, 1, 1,
				Nz::ShaderBindingType::Sampler,
				nzsl::ShaderStageType::Fragment
			},
			{
				0, 2, 1,
				Nz::ShaderBindingType::Sampler,
				nzsl::ShaderStageType::Fragment
			}
		});

		m_renderPipelineLayout = renderDevice->InstantiateRenderPipelineLayout(std::move(layoutInfo));
		if (!m_renderPipelineLayout)
			throw std::runtime_error("failed to instantiate atmosphere postprocess RenderPipelineLayout");

		m_onShaderUpdated.Connect(m_shader.OnShaderUpdated, [this](Nz::UberShader*)
		{
			BuildPipeline();
		});
		BuildPipeline();
	}

	void AtmosphereScatteringPipelinePass::Prepare(FrameData& frameData)
	{
		if (m_nextRenderPipeline)
		{
			if (m_renderPipeline)
				frameData.renderResources.PushForRelease(std::move(m_renderPipeline));

			m_renderPipeline = std::move(m_nextRenderPipeline);
			m_rebuildFramePass = true;
		}

		//TODO(?): update UBO only when camera moves?
		std::size_t bufferIndex;
		auto [renderBuffer, renderBufferView] = m_passDataBufferPool->Allocate(bufferIndex);

		auto& viewerInstance = m_viewer->GetViewerInstance();

		Nz::Vector3f eyePosition = viewerInstance.GetEyePosition();

		auto& allocation = frameData.renderResources.GetUploadPool().Allocate(AtmosphereScatteringPassFields.totalSize);
		Nz::AccessByOffset<Nz::Matrix4f&>(allocation.mappedPtr, AtmosphereScatteringPassFields.invProjectionMatrixOffset) = viewerInstance.GetInvProjectionMatrix();
		Nz::AccessByOffset<Nz::Matrix4f&>(allocation.mappedPtr, AtmosphereScatteringPassFields.invViewMatrixOffset) = viewerInstance.GetInvViewMatrix();
		Nz::AccessByOffset<Nz::Vector3f&>(allocation.mappedPtr, AtmosphereScatteringPassFields.viewerPositionOffset) = eyePosition;
		Nz::AccessByOffset<float&>(allocation.mappedPtr, AtmosphereScatteringPassFields.zNearOffset) = viewerInstance.GetNearPlane();

		m_cachedAtmospheres.clear();

		auto view = m_registry.view<Nz::NodeComponent, AtmosphereScattering>();
		for (auto&& [entity, atmosphereNode, atmosphereScattering] : view.each())
		{
			// TODO: Cull atmospheres
			m_cachedAtmospheres.emplace_back(atmosphereNode.GetGlobalPosition(), &atmosphereScattering);
		}

		std::sort(m_cachedAtmospheres.begin(), m_cachedAtmospheres.end(), [&](const auto& lhs, const auto& rhs)
		{
			return lhs.first.SquaredDistance(eyePosition) < rhs.first.SquaredDistance(eyePosition);
		});

		if (m_cachedAtmospheres.size() > MaxAtmosphereCount)
			m_cachedAtmospheres.resize(MaxAtmosphereCount);

		Nz::AccessByOffset<Nz::UInt32&>(allocation.mappedPtr, AtmosphereScatteringPassFields.atmosphereCountOffset) = Nz::SafeCaster(m_cachedAtmospheres.size());

		Nz::UInt8* atmosphereBasePtr = static_cast<Nz::UInt8*>(allocation.mappedPtr) + AtmosphereScatteringPassFields.atmospheresOffset;
		for (auto rit = m_cachedAtmospheres.rbegin(); rit != m_cachedAtmospheres.rend(); ++rit)
		{
			auto&& [planetPos, atmosphereScattering] = *rit;

			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.planetPositionOffset) = planetPos;

			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.sunDirOffset) = atmosphereScattering->sunDir;
			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.sunIntensityOffset) = atmosphereScattering->sunIntensity;
			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.planetDimensionsOffset) = atmosphereScattering->planetDimensions;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.planetCornerRadiusOffset) = atmosphereScattering->planetCornerRadius;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.atmosphereMaxHeightOffset) = atmosphereScattering->atmosphereMaxHeight;

			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.rayleighBetaOffset) = atmosphereScattering->rayleighBeta;
			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.mieBetaOffset) = atmosphereScattering->mieBeta;
			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.ambientBetaOffset) = atmosphereScattering->ambientBeta;
			Nz::AccessByOffset<Nz::Vector3f&>(atmosphereBasePtr, AtmosphereScatteringFields.absorptionBetaOffset) = atmosphereScattering->absorptionBeta;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.mieScatteringDirectionOffset) = atmosphereScattering->mieScattering;

			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.rayleighHeightOffset) = atmosphereScattering->rayleighHeight;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.mieHeightOffset) = atmosphereScattering->mieHeight;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.heightAbsorptionOffset) = atmosphereScattering->heightAbsorption;
			Nz::AccessByOffset<float&>(atmosphereBasePtr, AtmosphereScatteringFields.absorptionFalloffOffset) = atmosphereScattering->absorptionFalloff;

			Nz::AccessByOffset<Nz::Int32&>(atmosphereBasePtr, AtmosphereScatteringFields.primaryStepsOffset) = atmosphereScattering->primarySteps;
			Nz::AccessByOffset<Nz::Int32&>(atmosphereBasePtr, AtmosphereScatteringFields.lightStepsOffset) = atmosphereScattering->lightSteps;

			atmosphereBasePtr += AtmosphereScatteringFields.totalSize;
		}

		frameData.renderResources.Execute([&](Nz::CommandBufferBuilder& builder)
		{
			builder.CopyBuffer(allocation, renderBufferView);
			builder.MemoryBarrier(Nz::PipelineStage::Transfer, Nz::PipelineStage::FragmentShader, Nz::MemoryAccess::TransferWrite, Nz::MemoryAccess::UniformBufferRead);
		}, Nz::QueueType::Transfer);

		frameData.renderResources.PushReleaseCallback([pool = m_passDataBufferPool, bufferIndex]
		{
			pool->Free(bufferIndex);
		});

		m_passDataBuffer = renderBufferView;
	}

	Nz::FramePass& AtmosphereScatteringPipelinePass::RegisterToFrameGraph(Nz::FrameGraph& frameGraph, const PassInputOuputs& inputOuputs)
	{
		if (inputOuputs.inputAttachments.size() != 2)
			throw std::runtime_error("two inputs expected");

		if (inputOuputs.outputAttachments.size() != 1)
			throw std::runtime_error("one output expected");

		if (inputOuputs.depthStencilInput != InvalidAttachmentIndex)
			throw std::runtime_error("unexpected depth-stencil output");

		if (inputOuputs.depthStencilOutput != InvalidAttachmentIndex)
			throw std::runtime_error("unexpected depth-stencil output");

		std::size_t inputColorBufferIndex = inputOuputs.inputAttachments[0].attachmentIndex;
		std::size_t inputDepthBufferIndex = inputOuputs.inputAttachments[1].attachmentIndex;

		Nz::FramePass& postProcess = frameGraph.AddPass(m_passName);
		postProcess.AddInput(inputColorBufferIndex);
		postProcess.AddInput(inputDepthBufferIndex);
		postProcess.AddOutput(inputOuputs.outputAttachments[0].attachmentIndex);

		postProcess.SetInputAccess(1, Nz::TextureLayout::DepthStencilReadOnly, Nz::PipelineStage::FragmentShader, Nz::MemoryAccess::ShaderRead);

		postProcess.SetExecutionCallback([&]
		{
			return Nz::FramePassExecution::UpdateAndExecute;
		});

		postProcess.SetCommandCallback([this, inputColorBufferIndex, inputDepthBufferIndex](Nz::CommandBufferBuilder& builder, const Nz::FramePassEnvironment& env)
		{
			auto& samplerCache = Nz::Graphics::Instance()->GetSamplerCache();

			const auto& colorTexture = env.frameGraph.GetAttachmentTexture(inputColorBufferIndex);
			const auto& depthTexture = env.frameGraph.GetAttachmentTexture(inputDepthBufferIndex);
			const auto& sampler = samplerCache.Get({});

			Nz::ShaderBindingPtr shaderBinding = m_renderPipelineLayout->AllocateShaderBinding(0);
			shaderBinding->Update({
				{
					0,
					Nz::ShaderBinding::UniformBufferBinding {
						m_passDataBuffer.GetBuffer(), m_passDataBuffer.GetOffset(), m_passDataBuffer.GetSize()
					}
				},
				{
					1,
					Nz::ShaderBinding::SampledTextureBinding {
						colorTexture.get(), sampler.get()
					}
				},
				{
					2,
					Nz::ShaderBinding::SampledTextureBinding {
						depthTexture.get(), sampler.get(), Nz::TextureLayout::DepthStencilReadOnly
					}
				}
			});

			builder.SetScissor(env.renderRect);
			builder.SetViewport(env.renderRect);

			builder.BindRenderPipeline(*m_renderPipeline);
			builder.BindRenderShaderBinding(0, *shaderBinding);

			builder.Draw(3);

			if (shaderBinding)
				env.renderResources.PushForRelease(std::move(shaderBinding));

			m_rebuildFramePass = false;
		});

		return postProcess;
	}

	void AtmosphereScatteringPipelinePass::Register(entt::registry& registry)
	{
		Nz::FramePipelinePassRegistry& passRegistry = Nz::Graphics::Instance()->GetFramePipelinePassRegistry();
		passRegistry.RegisterPass<AtmosphereScatteringPipelinePass>("TSOM_AtmosphereScattering", { "Color", "Depth" }, { "Output" }, std::ref(registry));
	}

	void AtmosphereScatteringPipelinePass::BuildPipeline()
	{
		std::shared_ptr<Nz::RenderDevice> renderDevice = Nz::Graphics::Instance()->GetRenderDevice();

		Nz::UberShader::Config config;
		config.optionValues[nzsl::Ast::HashOption("MaxAtmosphereCount")] = MaxAtmosphereCount;

		Nz::RenderPipelineInfo pipelineInfo;
		pipelineInfo.pipelineLayout = m_renderPipelineLayout;
		pipelineInfo.shaderModules.push_back(m_shader.Get(config));

		m_nextRenderPipeline = renderDevice->InstantiateRenderPipeline(pipelineInfo);
	}
}
