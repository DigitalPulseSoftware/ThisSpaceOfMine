// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_ENTITIES_SERVERCLASSLIBRARY_HPP
#define TSOM_SERVERLIB_ENTITIES_SERVERCLASSLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class BlockLibrary;
	class ChunkContainer;
	class ChunkEntities;

	class TSOM_SERVERLIB_API ServerClassLibrary : public EntityClassLibrary
	{
		public:
			inline ServerClassLibrary(Nz::ApplicationBase& app);
			ServerClassLibrary(const ServerClassLibrary&) = delete;
			ServerClassLibrary(ServerClassLibrary&&) = delete;
			~ServerClassLibrary() = default;

			void Register(EntityRegistry& registry) override;

			ServerClassLibrary& operator=(const ServerClassLibrary&) = delete;
			ServerClassLibrary& operator=(ServerClassLibrary&&) = delete;

		protected:
			Nz::ApplicationBase& m_app;
	};
}

#include <ServerLib/Entities/ServerClassLibrary.inl>

#endif // TSOM_SERVERLIB_ENTITIES_SERVERCLASSLIBRARY_HPP
