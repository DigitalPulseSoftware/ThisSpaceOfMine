// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_ENTITIES_SERVERENTITYCLASSLIBRARY_HPP
#define TSOM_SERVERLIB_ENTITIES_SERVERENTITYCLASSLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Entities/SharedEntityClassLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class BlockLibrary;
	class ChunkContainer;
	class ChunkEntities;

	class TSOM_SERVERLIB_API ServerEntityClassLibrary final : public SharedEntityClassLibrary
	{
		public:
			using SharedEntityClassLibrary::SharedEntityClassLibrary;

		protected:
			void OnPlayerActivate(entt::handle entity) override;
			void OnPlayerInit(entt::handle entity) override;
			void OnShipExteriorActivate(entt::handle entity) override;
			void OnShipExteriorInit(entt::handle entity) override;
	};
}

#include <ServerLib/Entities/ServerEntityClassLibrary.inl>

#endif // TSOM_SERVERLIB_ENTITIES_SERVERENTITYCLASSLIBRARY_HPP
