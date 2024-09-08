// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYREGISTRY_HPP
#define TSOM_COMMONLIB_ENTITYREGISTRY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityClass.hpp>
#include <tsl/hopscotch_map.h>
#include <string>

namespace tsom
{
	class TSOM_COMMONLIB_API EntityRegistry
	{
		public:
			EntityRegistry() = default;
			EntityRegistry(const EntityRegistry&) = delete;
			EntityRegistry(EntityRegistry&&) = delete;
			~EntityRegistry() = default;

			inline const EntityClass* FindClass(std::string_view entityClass) const;

			template<typename F> void ForEachClass(F&& functor);
			template<typename F> void ForEachClass(F&& functor) const;

			void RegisterClass(EntityClass entityClass);

			EntityRegistry& operator=(const EntityRegistry&) = delete;
			EntityRegistry& operator=(EntityRegistry&&) = delete;

		private:
			tsl::hopscotch_map<std::string, EntityClass, std::hash<std::string_view>, std::equal_to<>> m_classes;
	};
}

#include <CommonLib/EntityRegistry.inl>

#endif // TSOM_COMMONLIB_ENTITYREGISTRY_HPP
