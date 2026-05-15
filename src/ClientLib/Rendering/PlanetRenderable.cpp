// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Rendering/PlanetRenderable.hpp>
#include <ClientLib/Rendering/RenderPlanetLayer.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <Nazara/Graphics/ElementRendererRegistry.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>

namespace tsom
{
	PlanetRenderable::PlanetRenderable(const ClientBlockLibrary& blockLibrary, const ChunkContainer& chunkContainer) :
	m_blockLibrary(blockLibrary),
	m_chunkContainer(chunkContainer)
	{
		m_onAABBUpdated.Connect(chunkContainer.OnAABBUpdated, [this](const ChunkContainer*)
		{
			UpdateAABB(m_chunkContainer.GetAABB());
		});

		UpdateAABB(chunkContainer.GetAABB());
	}

	void PlanetRenderable::BuildElement(Nz::ElementRendererRegistry& registry, const ElementData& elementData, std::size_t passIndex, std::vector<Nz::RenderElementOwner>& elements) const
	{
		for (std::size_t layerIndex = 0; layerIndex < m_layers.size(); ++layerIndex)
		{
			const auto& chunkEntities = m_layers[layerIndex];
			if (!chunkEntities)
				continue;

			const std::shared_ptr<Nz::MaterialInstance>& layerMaterial = chunkEntities->GetMaterial();

			const auto& materialPipeline = layerMaterial->GetPipeline(passIndex);
			if (!materialPipeline)
				continue;

			Nz::RenderPipelineInfo::VertexBufferData vertexBufferData = {
				0,
				chunkEntities->GetVertexDeclaration()
			};

			const auto& renderPipeline = materialPipeline->GetRenderPipeline(&vertexBufferData, 1);

			Nz::MaterialPassFlags passFlags = layerMaterial->GetPassFlags(passIndex);

			const auto& layerData = m_blockLibrary.GetLayerData(layerIndex);
			elements.emplace_back(registry.AllocateElement<RenderPlanetLayer>(layerData.renderLayer, passFlags, renderPipeline, *elementData.worldInstance, *elementData.scissorBox, *chunkEntities));
		}
	}

	const std::shared_ptr<Nz::MaterialInstance>& PlanetRenderable::GetMaterial(std::size_t materialIndex) const
	{
		NazaraAssert(materialIndex < m_layers.size());
		const auto& chunkEntities = m_layers[materialIndex];
		if (!chunkEntities)
		{
			static std::shared_ptr<Nz::MaterialInstance> s_invalidMaterial;
			return s_invalidMaterial;
		}

		return chunkEntities->GetMaterial();
	}

	std::size_t PlanetRenderable::GetMaterialCount() const
	{
		return m_layers.size();
	}

	void PlanetRenderable::RegisterLayer(std::shared_ptr<ClientChunkEntities> chunkLayer)
	{
		std::size_t layerIndex = chunkLayer->GetLayerIndex();
		OnMaterialInvalidated(this, layerIndex, chunkLayer->GetMaterial());
		m_layers[layerIndex] = std::move(chunkLayer);
	}
}
