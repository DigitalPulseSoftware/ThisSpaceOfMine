// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ENTITIES_CLIENTENTITYCLASSLIBRARY_HPP
#define TSOM_CLIENTLIB_ENTITIES_CLIENTENTITYCLASSLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Entities/SharedEntityClassLibrary.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class BlockLibrary;
	class ChunkContainer;
	class ChunkEntities;

	class TSOM_CLIENTLIB_API ClientEntityClassLibrary final : public SharedEntityClassLibrary
	{
		public:
			using SharedEntityClassLibrary::SharedEntityClassLibrary;

		private:
			void OnPlayerActivate(entt::handle entity) override;
			void OnPlayerRpc_Death(entt::handle entity) override;
	};
}

#include <ClientLib/Entities/ClientEntityClassLibrary.inl>

#endif // TSOM_CLIENTLIB_ENTITIES_CLIENTENTITYCLASSLIBRARY_HPP
