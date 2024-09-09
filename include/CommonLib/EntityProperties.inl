// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
#define TSOM_ENTITYPROPERTYTYPE(V, T, UT) \
	template<> \
	struct EntityPropertyUnderlyingType< EntityPropertyType:: T > \
	{ \
		using UnderlyingType = UT; \
	};

#include <CommonLib/EntityPropertyList.hpp>

	template<EntityPropertyType P>
	struct EntityPropertyTypeExtractor<EntityPropertySingleValue<P>>
	{
		static constexpr EntityPropertyType Property = P;
		static constexpr bool IsArray = false;

		using UnderlyingType = EntityPropertyUnderlyingType_t<P>;
	};

	template<EntityPropertyType P>
	struct EntityPropertyTypeExtractor<EntityPropertyArrayValue<P>>
	{
		static constexpr EntityPropertyType Property = P;
		static constexpr bool IsArray = true;

		using UnderlyingType = EntityPropertyUnderlyingType_t<P>;
	};

	template<EntityPropertyType P>
	EntityPropertyArrayValue<P>::EntityPropertyArrayValue(std::size_t elementCount) :
	m_size(elementCount)
	{
		m_arrayData = std::make_unique<UnderlyingType[]>(m_size);
	}

	template<EntityPropertyType P>
	EntityPropertyArrayValue<P>::EntityPropertyArrayValue(const EntityPropertyArrayValue& container)
	{
		*this = container;
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::GetElement(std::size_t i) -> UnderlyingType&
	{
		assert(i < m_size);
		return m_arrayData[i];
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::GetElement(std::size_t i) const -> const UnderlyingType&
	{
		assert(i < m_size);
		return m_arrayData[i];
	}

	template<EntityPropertyType P>
	std::size_t EntityPropertyArrayValue<P>::GetSize() const
	{
		return m_size;
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::operator[](std::size_t i) -> UnderlyingType&
	{
		return GetElement(i);
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::operator[](std::size_t i) const -> const UnderlyingType&
	{
		return GetElement(i);
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::begin() -> UnderlyingType*
	{
		return &m_arrayData[0];
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::begin() const -> const UnderlyingType*
	{
		return &m_arrayData[0];
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::end() -> UnderlyingType*
	{
		return &m_arrayData[m_size];
	}

	template<EntityPropertyType P>
	auto EntityPropertyArrayValue<P>::end() const -> const UnderlyingType*
	{
		return &m_arrayData[m_size];
	}

	template<EntityPropertyType P>
	std::size_t EntityPropertyArrayValue<P>::size() const
	{
		return GetSize();
	}

	template<EntityPropertyType P>
	EntityPropertyArrayValue<P>& EntityPropertyArrayValue<P>::operator=(const EntityPropertyArrayValue& container)
	{
		if (this == &container)
			return *this;

		m_size = container.m_size;
		m_arrayData = std::make_unique<UnderlyingType[]>(m_size);
		std::copy(container.m_arrayData.get(), container.m_arrayData.get() + m_size, m_arrayData.get());

		return *this;
	}


	template<EntityPropertyType P>
	EntityPropertySingleValue<P>::EntityPropertySingleValue(const UnderlyingType& v) :
	value(v)
	{
	}

	template<EntityPropertyType P>
	EntityPropertySingleValue<P>::EntityPropertySingleValue(UnderlyingType&& v) :
	value(std::move(v))
	{
	}

	template<EntityPropertyType P>
	auto EntityPropertySingleValue<P>::operator*() & -> UnderlyingType&
	{
		return value;
	}

	template<EntityPropertyType P>
	auto EntityPropertySingleValue<P>::operator*() && -> UnderlyingType&&
	{
		return std::move(value);
	}

	template<EntityPropertyType P>
	auto EntityPropertySingleValue<P>::operator*() const & -> const UnderlyingType&
	{
		return value;
	}
}
