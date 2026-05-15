// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_RENDERING_RENDERPLANET_HPP
#define TSOM_CLIENTLIB_RENDERING_RENDERPLANET_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Graphics/RenderElement.hpp>

namespace Nz
{
	class MaterialInstance;
	class RenderPipeline;
	class WorldInstance;
}

namespace tsom
{
	class ClientChunkEntities;

	class TSOM_CLIENTLIB_API RenderPlanetLayer : public Nz::RenderElement
	{
		public:
			inline RenderPlanetLayer(int renderLayer, Nz::MaterialPassFlags materialFlags, std::shared_ptr<Nz::RenderPipeline> renderPipeline, const Nz::WorldInstance& worldInstance, const Nz::Recti& scissorBox, ClientChunkEntities& clientChunkEntities);
			RenderPlanetLayer(const RenderPlanetLayer&) = delete;
			RenderPlanetLayer(RenderPlanetLayer&&) = delete;
			~RenderPlanetLayer() = default;

			Nz::UInt64 ComputeSortingScore(const Nz::Frustumf& frustum, const Nz::RenderQueueRegistry& registry) const override;

			inline ClientChunkEntities& GetClientChunkEntities() const;
			inline const Nz::RenderPipeline* GetRenderPipeline() const;
			inline const Nz::Recti& GetScissorBox() const;
			inline const Nz::WorldInstance& GetWorldInstance() const;

			void Register(Nz::RenderQueueRegistry& registry) const override;

			RenderPlanetLayer& operator=(const RenderPlanetLayer&) = delete;
			RenderPlanetLayer& operator=(RenderPlanetLayer&&) = delete;

			static constexpr Nz::UInt8 ElementType = Nz::BasicRenderElementCount + 0;

		private:
			std::shared_ptr<Nz::RenderPipeline> m_renderPipeline;
			const Nz::WorldInstance& m_worldInstance;
			ClientChunkEntities& m_clientChunkEntities;
			Nz::MaterialPassFlags m_materialFlags;
			Nz::Recti m_scissorBox;
			int m_renderLayer;

	};
}

#include <ClientLib/Rendering/RenderPlanetLayer.inl>

#endif // TSOM_CLIENTLIB_RENDERING_RENDERPLANET_HPP
