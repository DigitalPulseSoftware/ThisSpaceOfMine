// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/PacketSerializer.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <CommonLib/Protocol/CompressedInteger.hpp>
#include <cassert>
#include <stdexcept>

namespace tsom
{
	namespace Detail
	{
		template<typename T>
		struct RuntimeToTemplateInteger;

		template<std::size_t N, std::size_t... Indices>
		struct RuntimeToTemplateInteger<std::integer_sequence<std::size_t, N, Indices...>>
		{
			template<template<std::size_t> typename T, typename... Args>
			static void Select(std::size_t index, Args&&... args)
			{
				if (index == N)
					T<N>{}(std::forward<Args>(args)...);
				else if constexpr (sizeof...(Indices) > 0)
					RuntimeToTemplateInteger<std::integer_sequence<std::size_t, Indices...>>::template Select<T>(index, std::forward<Args>(args)...);
			}
		};

		template<std::size_t N>
		struct VariantSerializeByIndex
		{
			template<typename F, typename... Args>
			void operator()(std::variant<Args...>& variant, bool isWriting, F&& func)
			{
				using T = std::variant_alternative_t<N, std::variant<Args...>>;

				if (isWriting)
					func(std::get<T>(variant));
				else
					func(variant.template emplace<T>());
			}
		};
	}

	inline PacketSerializer::PacketSerializer(Nz::ByteStream& packetBuffer, bool isWriting) :
	m_buffer(packetBuffer),
	m_isWriting(isWriting)
	{
	}

	inline void PacketSerializer::Read(void* ptr, std::size_t size)
	{
		if (m_buffer.Read(ptr, size) != size)
			throw std::runtime_error("failed to read");
	}

	inline bool PacketSerializer::IsWriting() const
	{
		return m_isWriting;
	}

	inline void PacketSerializer::Write(const void* ptr, std::size_t size)
	{
		if (m_buffer.Write(ptr, size) != size)
			throw std::runtime_error("failed to write");
	}

	template<typename DataType>
	void PacketSerializer::Serialize(DataType& data)
	{
		if (!IsWriting())
			m_buffer >> data;
		else
			m_buffer << data;
	}

	template<typename F, typename... Types>
	void PacketSerializer::Serialize(std::variant<Types...>& variant, F&& functor)
	{
		Nz::UInt8 index;
		if (!IsWriting())
			Serialize(index);
		else
		{
			index = Nz::SafeCast<Nz::UInt8>(variant.index());
			Serialize(index);
		}

		using Sequence = std::make_integer_sequence<std::size_t, sizeof...(Types)>;
		using TemplateInt = Detail::RuntimeToTemplateInteger<Sequence>;

		TemplateInt::template Select<Detail::VariantSerializeByIndex>(index, variant, IsWriting(), functor);
	}

	template<typename DataType>
	void PacketSerializer::Serialize(std::vector<DataType>& dataVec)
	{
		SerializeArraySize(dataVec);
		for (DataType& data : dataVec)
			Serialize(data);
	}

	template<typename DataType>
	void PacketSerializer::Serialize(const DataType& data) const
	{
		assert(IsWriting());

		m_buffer << data;
	}

	template<typename PacketType, typename DataType>
	void PacketSerializer::Serialize(DataType& data)
	{
		if (!IsWriting())
		{
			PacketType packetData;
			m_buffer >> packetData;

			data = static_cast<DataType>(packetData);
		}
		else
			m_buffer << static_cast<PacketType>(data);
	}

	template<typename PacketType, typename DataType>
	void PacketSerializer::Serialize(const DataType& data) const
	{
		assert(IsWriting());

		m_buffer << static_cast<PacketType>(data);
	}

	template<typename T>
	void PacketSerializer::SerializeArraySize(T& array)
	{
		CompressedUnsigned<Nz::UInt32> arraySize;
		if (IsWriting())
			arraySize = Nz::UInt32(array.size());

		Serialize(arraySize);

		if (!IsWriting())
			array.resize(arraySize);
	}

	template<typename T>
	void PacketSerializer::SerializeArraySize(const T& array)
	{
		assert(IsWriting());

		CompressedUnsigned<Nz::UInt32> arraySize(Nz::UInt32(array.size()));
		Serialize(arraySize);
	}

	template<typename E, typename UT>
	void PacketSerializer::SerializeEnum(E& enumValue)
	{
		if (IsWriting())
			m_buffer << static_cast<UT>(enumValue);
		else
		{
			UT v;
			m_buffer >> v;

			enumValue = static_cast<E>(v);
		}
	}

	template<typename DataType>
	void PacketSerializer::operator&=(DataType& data)
	{
		return Serialize(data);
	}

	template<typename DataType>
	void PacketSerializer::operator&=(const DataType& data) const
	{
		return Serialize(data);
	}
}
