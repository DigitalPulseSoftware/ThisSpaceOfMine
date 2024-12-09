// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline EntityReference::EntityReference() :
	m_handleData(GetEmptyHandle())
	{
	}

	inline EntityReference::EntityReference(std::shared_ptr<HandleData>&& handleData) :
	m_handleData(std::move(handleData))
	{
	}

	inline EntityReference::EntityReference(EntityReference&& reference) noexcept :
	m_handleData(std::move(reference.m_handleData))
	{
		reference.m_handleData = GetEmptyHandle();
	}

	inline entt::handle EntityReference::GetEntity()
	{
		return m_handleData->entity;
	}

	inline const entt::handle EntityReference::GetEntity() const
	{
		return m_handleData->entity;
	}

	inline bool EntityReference::IsValid() const
	{
		return static_cast<bool>(m_handleData->entity);
	}

	inline EntityReference::operator bool() const
	{
		return IsValid();
	}

	inline EntityReference::operator entt::handle() const
	{
		return m_handleData->entity;
	}

	inline entt::handle* EntityReference::operator->()
	{
		return &m_handleData->entity;
	}

	inline const entt::handle* EntityReference::operator->() const
	{
		return &m_handleData->entity;
	}

	inline EntityReference& EntityReference::operator=(EntityReference&& reference) noexcept
	{
		m_handleData = std::move(reference.m_handleData);
		reference.m_handleData = GetEmptyHandle();

		return *this;
	}
}
