// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYREGISTRY_HPP
#define TSOM_COMMONLIB_ENTITYREGISTRY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityClass.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <entt/fwd.hpp>
#include <tsl/hopscotch_map.h>
#include <span>
#include <string>

namespace tsom
{
	class EntityClassLibrary;

	class TSOM_COMMONLIB_API EntityRegistry
	{
		public:
			EntityRegistry();
			EntityRegistry(const EntityRegistry&) = delete;
			EntityRegistry(EntityRegistry&&) = delete;
			~EntityRegistry();

			inline std::shared_ptr<const EntityClass> FindClass(std::string_view entityClass) const;

			template<typename F> void ForEachClass(F&& functor);
			template<typename F> void ForEachClass(F&& functor) const;

			void Refresh(std::span<entt::registry*> registries, Nz::FunctionRef<void()> refreshCallback);

			void RegisterClass(EntityClass entityClass);

			template<typename T, typename... Args> void RegisterClassLibrary(Args&&... args);
			void RegisterClassLibrary(std::unique_ptr<EntityClassLibrary>&& library);

			EntityRegistry& operator=(const EntityRegistry&) = delete;
			EntityRegistry& operator=(EntityRegistry&&) = delete;

		private:
			std::vector<std::unique_ptr<EntityClassLibrary>> m_classLibraries;
			tsl::hopscotch_map<std::string, std::shared_ptr<EntityClass>, std::hash<std::string_view>, std::equal_to<>> m_classes;
			tsl::hopscotch_map<const EntityClass*, std::shared_ptr<const EntityClass>> m_refreshMap;
			bool m_isRefreshing;
	};
}

#include <CommonLib/EntityRegistry.inl>

#endif // TSOM_COMMONLIB_ENTITYREGISTRY_HPP
