// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline RenderPlanetLayer::RenderPlanetLayer(int renderLayer, Nz::MaterialPassFlags materialFlags, std::shared_ptr<Nz::RenderPipeline> renderPipeline, const Nz::WorldInstance& worldInstance, const Nz::Recti& scissorBox, ClientChunkEntities& clientChunkEntities) :
	RenderElement(ElementType),
	m_renderPipeline(std::move(renderPipeline)),
	m_worldInstance(worldInstance),
	m_clientChunkEntities(clientChunkEntities),
	m_materialFlags(materialFlags),
	m_scissorBox(scissorBox),
	m_renderLayer(renderLayer)
	{
	}

	inline ClientChunkEntities& RenderPlanetLayer::GetClientChunkEntities() const
	{
		return m_clientChunkEntities;
	}

	inline const Nz::RenderPipeline* RenderPlanetLayer::GetRenderPipeline() const
	{
		return m_renderPipeline.get();
	}

	inline const Nz::Recti& RenderPlanetLayer::GetScissorBox() const
	{
		return m_scissorBox;
	}

	inline const Nz::WorldInstance& RenderPlanetLayer::GetWorldInstance() const
	{
		return m_worldInstance;
	}
}
