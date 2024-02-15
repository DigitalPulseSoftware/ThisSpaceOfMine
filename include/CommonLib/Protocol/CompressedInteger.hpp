// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_COMPRESSEDINTEGER_HPP
#define TSOM_COMMONLIB_PROTOCOL_COMPRESSEDINTEGER_HPP

#include <Nazara/Core/Algorithm.hpp>
#include <type_traits>

namespace tsom
{
	template<typename T>
	class CompressedSigned
	{
		static_assert(std::is_signed_v<T>);

		public:
			explicit CompressedSigned(T value = 0);
			~CompressedSigned() = default;

			operator T() const;

			CompressedSigned& operator=(T value);

			CompressedSigned& operator++();
			CompressedSigned operator++(int);
			CompressedSigned& operator--();
			CompressedSigned operator--(int);

		private:
			T m_value;
	};

	template<typename T>
	class CompressedUnsigned
	{
		static_assert(std::is_unsigned_v<T>);

		public:
			explicit CompressedUnsigned(T value = 0);
			~CompressedUnsigned() = default;

			operator T() const;

			CompressedUnsigned& operator=(T value);

			CompressedUnsigned& operator++();
			CompressedUnsigned operator++(int);
			CompressedUnsigned& operator--();
			CompressedUnsigned operator--(int);

		private:
			T m_value;
	};
}

namespace Nz
{
	template<typename T> bool Serialize(SerializationContext& context, tsom::CompressedSigned<T> value, TypeTag<tsom::CompressedSigned<T>>);
	template<typename T> bool Serialize(SerializationContext& context, tsom::CompressedUnsigned<T> value, TypeTag<tsom::CompressedUnsigned<T>>);
	template<typename T> bool Unserialize(SerializationContext& context, tsom::CompressedSigned<T>* value, TypeTag<tsom::CompressedSigned<T>>);
	template<typename T> bool Unserialize(SerializationContext& context, tsom::CompressedUnsigned<T>* value, TypeTag<tsom::CompressedUnsigned<T>>);
}

#include <CommonLib/Protocol/CompressedInteger.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_COMPRESSEDINTEGER_HPP
