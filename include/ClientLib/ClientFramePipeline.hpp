// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTFRAMEPIPELINE_HPP
#define TSOM_CLIENTLIB_CLIENTFRAMEPIPELINE_HPP

#include <Nazara/Graphics/DefaultFramePipeline.hpp>
#include <ClientLib/Export.hpp>

namespace tsom
{
	class ClientBlockLibrary;

	class TSOM_CLIENTLIB_API ClientFramePipeline : public Nz::DefaultFramePipeline
	{
		public:
			inline ClientFramePipeline(Nz::ElementRendererRegistry& elementRegistry, ClientBlockLibrary& blockLibrary);
			ClientFramePipeline(const ClientFramePipeline&) = delete;
			ClientFramePipeline(ClientFramePipeline&&) = delete;
			~ClientFramePipeline() = default;

			void Render(Nz::RenderResources& renderResources) override;

			ClientFramePipeline& operator=(const ClientFramePipeline&) = delete;
			ClientFramePipeline& operator=(ClientFramePipeline&&) = delete;

		private:
			ClientBlockLibrary& m_blockLibrary;
	};
}

#include <ClientLib/ClientFramePipeline.inl>

#endif // TSOM_CLIENTLIB_CLIENTFRAMEPIPELINE_HPP
