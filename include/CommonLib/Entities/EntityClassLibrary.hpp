// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITIES_ENTITYCLASSLIBRARY_HPP
#define TSOM_COMMONLIB_ENTITIES_ENTITYCLASSLIBRARY_HPP

#include <CommonLib/Export.hpp>

namespace tsom
{
	class EntityRegistry;

	class TSOM_COMMONLIB_API EntityClassLibrary
	{
		public:
			EntityClassLibrary() = default;
			EntityClassLibrary(const EntityClassLibrary&) = delete;
			EntityClassLibrary(EntityClassLibrary&&) = delete;
			virtual ~EntityClassLibrary();

			virtual void Register(EntityRegistry& registry) = 0;

			EntityClassLibrary& operator=(const EntityClassLibrary&) = delete;
			EntityClassLibrary& operator=(EntityClassLibrary&&) = delete;
	};
}

#include <CommonLib/Entities/EntityClassLibrary.inl>

#endif // TSOM_COMMONLIB_ENTITIES_ENTITYCLASSLIBRARY_HPP
