// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITIES_SHAREDENTITYCLASSLIBRARY_HPP
#define TSOM_COMMONLIB_ENTITIES_SHAREDENTITYCLASSLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class ApplicationBase;
	class Collider3D;
}

namespace tsom
{
	class EntityClass;

	class TSOM_COMMONLIB_API SharedEntityClassLibrary : public EntityClassLibrary
	{
		public:
			SharedEntityClassLibrary(Nz::ApplicationBase& app);
			SharedEntityClassLibrary(const SharedEntityClassLibrary&) = delete;
			SharedEntityClassLibrary(SharedEntityClassLibrary&&) = delete;
			~SharedEntityClassLibrary() = default;

			void Register(EntityRegistry& registry) override;

			SharedEntityClassLibrary& operator=(const SharedEntityClassLibrary&) = delete;
			SharedEntityClassLibrary& operator=(SharedEntityClassLibrary&&) = delete;

		protected:
			virtual void OnPlayerActivate(entt::handle entity);
			virtual void OnPlayerInit(entt::handle entity);
			virtual void OnPlayerRpc_Death(entt::handle entity);
			virtual void OnShipExteriorActivate(entt::handle entity);
			virtual void OnShipExteriorInit(entt::handle entity);

			std::shared_ptr<Nz::Collider3D> m_playerCollider;
			std::shared_ptr<const EntityClass> m_playerClass;
			std::shared_ptr<const EntityClass> m_shipExteriorClass;
			Nz::ApplicationBase& m_app;
	};
}

#include <CommonLib/Entities/SharedEntityClassLibrary.inl>

#endif // TSOM_COMMONLIB_ENTITIES_SHAREDENTITYCLASSLIBRARY_HPP
