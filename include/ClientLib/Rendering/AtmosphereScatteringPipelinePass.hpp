// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_RENDERING_ATMOSPHERESCATTERINGPIPELINEPASS_HPP
#define TSOM_CLIENTLIB_RENDERING_ATMOSPHERESCATTERINGPIPELINEPASS_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/ParameterList.hpp>
#include <Nazara/Graphics/FramePipelinePass.hpp>
#include <Nazara/Graphics/UberShader.hpp>
#include <Nazara/Renderer/GpuBufferView.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class FrameGraph;
	class FramePass;
	class FramePipeline;
	class RenderFrame;
	class GpuBufferPool;
	class GpuPipeline;
}

namespace tsom
{
	struct AtmosphereScattering;

	class TSOM_CLIENTLIB_API AtmosphereScatteringPipelinePass : public Nz::FramePipelinePass
	{
		public:
			AtmosphereScatteringPipelinePass(PassData& passData, std::string passName, entt::registry& registry, const Nz::ParameterList& parameters);
			AtmosphereScatteringPipelinePass(const AtmosphereScatteringPipelinePass&) = delete;
			AtmosphereScatteringPipelinePass(AtmosphereScatteringPipelinePass&&) = delete;
			~AtmosphereScatteringPipelinePass() = default;

			void Prepare(FrameData& frameData) override;

			Nz::FramePass& RegisterToFrameGraph(Nz::FrameGraph& frameGraph, const PassInputOuputs& inputOuputs) override;

			AtmosphereScatteringPipelinePass& operator=(const AtmosphereScatteringPipelinePass&) = delete;
			AtmosphereScatteringPipelinePass& operator=(AtmosphereScatteringPipelinePass&&) = delete;

			static void Register(entt::registry& registry);

		private:
			void BuildPipeline();

			NazaraSlot(Nz::UberShader, OnShaderUpdated, m_onShaderUpdated);

			std::shared_ptr<Nz::GpuPipelineLayout> m_renderPipelineLayout;
			std::shared_ptr<Nz::GpuBufferPool> m_passDataBufferPool;
			std::shared_ptr<Nz::GpuRenderPipeline> m_renderPipeline;
			std::shared_ptr<Nz::GpuRenderPipeline> m_nextRenderPipeline;
			std::string m_passName;
			std::vector<std::pair<Nz::Vector3f, AtmosphereScattering*>> m_cachedAtmospheres;
			entt::registry& m_registry;
			Nz::AbstractViewer* m_viewer;
			Nz::GpuBufferView m_passDataBuffer;
			Nz::UberShader m_shader;
			bool m_rebuildFramePass;
	};
}

#include <ClientLib/Rendering/AtmosphereScatteringPipelinePass.inl>

#endif // TSOM_CLIENTLIB_RENDERING_ATMOSPHERESCATTERINGPIPELINEPASS_HPP
