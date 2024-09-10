// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP
#define TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Entities/ChunkClassLibrary.hpp>

namespace tsom
{
	class ClientBlockLibrary;

	class TSOM_CLIENTLIB_API ClientChunkClassLibrary final : public ChunkClassLibrary
	{
		public:
			ClientChunkClassLibrary(Nz::ApplicationBase& app, const ClientBlockLibrary& blockLibrary);

		private:
			void InitializeChunkEntity(entt::handle entity) override;
			std::unique_ptr<ChunkEntities> SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer) override;
	};
}

#include <ClientLib/Entities/ClientChunkClassLibrary.inl>

#endif // TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP
