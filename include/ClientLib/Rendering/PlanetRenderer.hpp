// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_RENDERING_PLANETRENDERER_HPP
#define TSOM_CLIENTLIB_RENDERING_PLANETRENDERER_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Graphics/ElementRenderer.hpp>
#include <Nazara/Graphics/RenderElementPool.hpp>
#include <Nazara/Renderer/ShaderBinding.hpp>
#include <CommonLib/Chunk.hpp>
#include <ClientLib/Rendering/RenderPlanetLayer.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API PlanetRenderer : public Nz::ElementRenderer
	{
		public:
			PlanetRenderer() = default;
			PlanetRenderer(const PlanetRenderer&) = delete;
			PlanetRenderer(PlanetRenderer&&) = delete;
			~PlanetRenderer() = default;

			Nz::RenderElementPoolBase& GetPool() override;

			std::unique_ptr<Nz::ElementRendererData> InstanciateData() override;

			void Render(const Nz::AbstractViewer& viewer, Nz::ElementRendererData& rendererData, Nz::RenderResources& currentFrame, Nz::CommandBufferBuilder& commandBuffer, std::size_t elementCount, const Nz::Pointer<const Nz::RenderElement>* elements, Nz::SparsePtr<const RenderStates> renderStates) override;

			PlanetRenderer& operator=(const PlanetRenderer&) = delete;
			PlanetRenderer& operator=(PlanetRenderer&&) = delete;

		private:
			std::vector<ChunkIndices> m_drawList;
			std::vector<Nz::ShaderBinding::Binding> m_bindingCache;
			std::vector<Nz::ShaderBinding::SampledTextureBinding> m_textureBindingCache;
			Nz::RenderElementPool<RenderPlanetLayer> m_pool;
	};

	struct PlanetRendererData : public Nz::ElementRendererData
	{
	};
}

#include <ClientLib/Rendering/PlanetRenderer.inl>

#endif // TSOM_CLIENTLIB_RENDERING_PLANETRENDERER_HPP
