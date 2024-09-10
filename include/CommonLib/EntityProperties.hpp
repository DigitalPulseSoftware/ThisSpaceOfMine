// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYPROPERTIES_HPP
#define TSOM_COMMONLIB_ENTITYPROPERTIES_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Serialization.hpp>
#include <Nazara/Math/Rect.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Math/Vector4.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <memory>
#include <optional>
#include <variant>

namespace tsom
{
	enum class EntityPropertyType
	{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) T = V,
#define TSOM_ENTITYPROPERTYTYPE_LAST(V, T, IT) T = V

#include <CommonLib/EntityPropertyList.hpp>
	};

	template<EntityPropertyType P>
	struct EntityPropertyUnderlyingType;

	template<EntityPropertyType P>
	using EntityPropertyUnderlyingType_t = typename EntityPropertyUnderlyingType<P>::UnderlyingType;


	template<typename>
	struct EntityPropertyTypeExtractor;

	template<EntityPropertyType P>
	struct EntityPropertyTag
	{
		static constexpr EntityPropertyType Property = P;
	};

	template<EntityPropertyType P>
	class EntityPropertyArrayValue
	{
		public:
			static constexpr EntityPropertyType Property = P;
			using UnderlyingType = EntityPropertyUnderlyingType_t<Property>;

			explicit EntityPropertyArrayValue(std::size_t elementCount);
			EntityPropertyArrayValue(const EntityPropertyArrayValue&);
			EntityPropertyArrayValue(EntityPropertyArrayValue&&) noexcept = default;
			~EntityPropertyArrayValue() = default;

			UnderlyingType& GetElement(std::size_t i);
			const UnderlyingType& GetElement(std::size_t i) const;
			std::size_t GetSize() const;

			UnderlyingType& operator[](std::size_t i);
			const UnderlyingType& operator[](std::size_t i) const;

			// To allow range-for loops
			UnderlyingType* begin();
			const UnderlyingType* begin() const;
			UnderlyingType* end();
			const UnderlyingType* end() const;
			std::size_t size() const;

			EntityPropertyArrayValue& operator=(const EntityPropertyArrayValue&);
			EntityPropertyArrayValue& operator=(EntityPropertyArrayValue&&) noexcept = default;

		private:
			std::size_t m_size;
			std::unique_ptr<UnderlyingType[]> m_arrayData;
	};

	template<EntityPropertyType P>
	struct EntityPropertySingleValue
	{
		static constexpr EntityPropertyType Property = P;
		using UnderlyingType = EntityPropertyUnderlyingType_t<Property>;

		EntityPropertySingleValue() = default;
		EntityPropertySingleValue(const UnderlyingType& v);
		EntityPropertySingleValue(UnderlyingType&& v);
		EntityPropertySingleValue(const EntityPropertySingleValue&) = default;
		EntityPropertySingleValue(EntityPropertySingleValue&&) noexcept = default;

		UnderlyingType& operator*() &;
		UnderlyingType&& operator*() &&;
		const UnderlyingType& operator*() const &;

		operator UnderlyingType&() &;
		operator UnderlyingType&&() &&;
		operator const UnderlyingType&() const &;

		EntityPropertySingleValue& operator=(const EntityPropertySingleValue&) = default;
		EntityPropertySingleValue& operator=(EntityPropertySingleValue&&) noexcept = default;

		UnderlyingType value;
	};

	using EntityProperty = std::variant<
#define TSOM_ENTITYPROPERTYTYPE_LAST(V, T, IT) EntityPropertySingleValue<EntityPropertyType:: T>, EntityPropertyArrayValue<EntityPropertyType:: T>
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) TSOM_ENTITYPROPERTYTYPE_LAST(V, T, IT),

#include <CommonLib/EntityPropertyList.hpp>
	>;

	TSOM_COMMONLIB_API std::pair<EntityPropertyType, bool> ExtractPropertyType(const EntityProperty& value);
	TSOM_COMMONLIB_API EntityPropertyType ParseEntityPropertyType(std::string_view str);
	TSOM_COMMONLIB_API std::string_view ToString(EntityPropertyType propertyType);
}

namespace Nz
{
	TSOM_COMMONLIB_API bool Deserialize(SerializationContext& context, tsom::EntityProperty* entityProperty, TypeTag<tsom::EntityProperty>);
	TSOM_COMMONLIB_API bool Serialize(SerializationContext& context, const tsom::EntityProperty& entityProperty, TypeTag<tsom::EntityProperty>);
}

#include <CommonLib/EntityProperties.inl>

#endif // TSOM_COMMONLIB_ENTITYPROPERTIES_HPP
