// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/EntityClass.hpp>
#include <fmt/format.h>

namespace tsom
{
	ClassInstanceComponent::ClassInstanceComponent(std::shared_ptr<const EntityClass> entityClass) :
	m_entityClass(std::move(entityClass))
	{
		std::size_t propertyCount = m_entityClass->GetPropertyCount();

		m_properties.reserve(propertyCount);
		for (std::size_t i = 0; i < propertyCount; ++i)
			m_properties.emplace_back(m_entityClass->GetProperty(i).defaultValue);
	}

	Nz::UInt32 ClassInstanceComponent::FindClientRpcIndex(std::string_view rpcName) const
	{
		return m_entityClass->FindClientRpc(rpcName);
	}

	Nz::UInt32 ClassInstanceComponent::FindPropertyIndex(std::string_view propertyName) const
	{
		return m_entityClass->FindProperty(propertyName);
	}

	Nz::UInt32 ClassInstanceComponent::GetPropertyIndex(std::string_view propertyName) const
	{
		Nz::UInt32 propertyIndex = FindPropertyIndex(propertyName);
		if (propertyIndex == EntityClass::InvalidIndex)
			throw std::runtime_error(fmt::format("invalid property {}", propertyName));

		return propertyIndex;
	}

	void ClassInstanceComponent::TriggerClientRpc(Nz::UInt32 rpcIndex, ServerPlayer* targetPlayer)
	{
		OnClientRpc(this, rpcIndex, targetPlayer);
	}

	void ClassInstanceComponent::UpdateClass(std::shared_ptr<const EntityClass> entityClass)
	{
		m_entityClass = std::move(entityClass);

		// TODO: Handle possible properties changes
	}
}
