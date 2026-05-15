// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_RENDERING_PLANETRENDERABLE_HPP
#define TSOM_CLIENTLIB_RENDERING_PLANETRENDERABLE_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Graphics/InstancedRenderable.hpp>

namespace tsom
{
	class ClientBlockLibrary;
	class ClientChunkEntities;

	class TSOM_CLIENTLIB_API PlanetRenderable : public Nz::InstancedRenderable
	{
		public:
			PlanetRenderable(const ClientBlockLibrary& blockLibrary, const ChunkContainer& chunkContainer);
			PlanetRenderable(const PlanetRenderable&) = delete;
			PlanetRenderable(PlanetRenderable&&) = delete;
			~PlanetRenderable() = default;

			void BuildElement(Nz::ElementRendererRegistry& registry, const ElementData& elementData, std::size_t passIndex, std::vector<Nz::RenderElementOwner>& elements) const override;

			const std::shared_ptr<Nz::MaterialInstance>& GetMaterial(std::size_t materialIndex) const override;
			std::size_t GetMaterialCount() const override;

			void RegisterLayer(std::shared_ptr<ClientChunkEntities> chunkLayer);

			PlanetRenderable& operator=(const PlanetRenderable&) = delete;
			PlanetRenderable& operator=(PlanetRenderable&&) = delete;

		private:
			NazaraSlot(ChunkContainer, OnAABBUpdated, m_onAABBUpdated);

			std::array<std::shared_ptr<ClientChunkEntities>, Constants::MaxChunkLayerCount> m_layers;
			const ClientBlockLibrary& m_blockLibrary;
			const ChunkContainer& m_chunkContainer;
	};
}

#include <ClientLib/Rendering/PlanetRenderable.inl>

#endif // TSOM_CLIENTLIB_RENDERING_PLANETRENDERABLE_HPP
