// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYREFERENCE_HPP
#define TSOM_COMMONLIB_ENTITYREFERENCE_HPP

#include <CommonLib/Export.hpp>
#include <entt/entt.hpp>
#include <memory>

namespace tsom
{
	class TSOM_COMMONLIB_API EntityReference
	{
		public:
			struct HandleData;
			struct HandleOwner;

			inline EntityReference();
			EntityReference(entt::handle entity);
			EntityReference(std::shared_ptr<HandleData>&& handleData);
			EntityReference(const EntityReference&) = default;
			EntityReference(EntityReference&& reference) noexcept;
			~EntityReference() = default;

			inline entt::handle GetEntity();
			inline const entt::handle GetEntity() const;

			inline bool IsValid() const;

			inline explicit operator bool() const;
			inline operator entt::handle() const;
			inline entt::handle* operator->();
			inline const entt::handle* operator->() const;

			EntityReference& operator=(const EntityReference&) = default;
			EntityReference& operator=(EntityReference&&) noexcept;

			struct HandleData
			{
				entt::handle entity;
			};

			struct HandleOwner
			{
				std::shared_ptr<HandleData> handleData;
			};

		private:
			static std::shared_ptr<HandleData> GetEmptyHandle();

			std::shared_ptr<HandleData> m_handleData;
	};
}

#include <CommonLib/EntityReference.inl>

#endif // TSOM_COMMONLIB_ENTITYREFERENCE_HPP
