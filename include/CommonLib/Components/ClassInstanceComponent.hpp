// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP

#include <CommonLib/EntityProperties.hpp>
#include <NazaraUtils/Signal.hpp>
#include <string_view>
#include <vector>

namespace tsom
{
	class EntityClass;
	class ServerPlayer;

	class TSOM_COMMONLIB_API ClassInstanceComponent
	{
		public:
			using PropertyUpdateSignal = Nz::Signal<Nz::UInt32, const EntityProperty&>;

			ClassInstanceComponent(std::shared_ptr<const EntityClass> entityClass);
			ClassInstanceComponent(const ClassInstanceComponent&) = delete;
			ClassInstanceComponent(ClassInstanceComponent&&) noexcept = default;
			~ClassInstanceComponent() = default;

			Nz::UInt32 FindClientRpcIndex(std::string_view rpcName) const;
			Nz::UInt32 FindPropertyIndex(std::string_view propertyName) const;

			inline const std::shared_ptr<const EntityClass>& GetClass() const;
			inline const EntityProperty& GetProperty(Nz::UInt32 propertyIndex) const;
			template<EntityPropertyType Property> auto GetProperty(Nz::UInt32 propertyIndex) const;
			template<EntityPropertyType Property> auto GetProperty(std::string_view propertyName) const;

			Nz::UInt32 GetPropertyIndex(std::string_view propertyName) const;

			void TriggerClientRpc(Nz::UInt32 rpcIndex, ServerPlayer* targetPlayer);

			void UpdateClass(std::shared_ptr<const EntityClass> entityClass);
			inline void UpdateProperty(Nz::UInt32 propertyIndex, EntityProperty&& value);
			template<EntityPropertyType Property, typename T> void UpdateProperty(Nz::UInt32 propertyIndex, T&& value);
			template<EntityPropertyType Property, typename T> void UpdateProperty(std::string_view propertyName, T&& value);

			ClassInstanceComponent& operator=(const ClassInstanceComponent&) = delete;
			ClassInstanceComponent& operator=(ClassInstanceComponent&&) noexcept = default;

			NazaraSignal(OnClientRpc, ClassInstanceComponent* /*classInstance*/, Nz::UInt32 /*rpcIndex*/, ServerPlayer* /*targetPlayer*/);
			NazaraSignal(OnPropertyUpdate, ClassInstanceComponent* /*classInstance*/, Nz::UInt32 /*propertyIndex*/, const EntityProperty& /*newValue*/);

		private:
			std::shared_ptr<const EntityClass> m_entityClass;
			std::vector<EntityProperty> m_properties;
	};
}

#include <CommonLib/Components/ClassInstanceComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP
