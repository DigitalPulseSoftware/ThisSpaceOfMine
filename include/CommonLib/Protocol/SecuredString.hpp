// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_SECUREDSTRING_HPP
#define TSOM_COMMONLIB_PROTOCOL_SECUREDSTRING_HPP

#include <Nazara/Core/Algorithm.hpp>
#include <limits>
#include <type_traits>

namespace tsom
{
	template<std::size_t N, bool CodepointLimit = true>
	class SecuredString
	{
		static_assert(N > 0);
		static_assert(N <= std::numeric_limits<Nz::UInt32>::max());

		public:
			using SizeType = std::conditional_t<
				N <= std::numeric_limits<Nz::UInt8>::max(), Nz::UInt8,
				std::conditional_t<
					N <= std::numeric_limits<Nz::UInt16>::max(), Nz::UInt16,
					Nz::UInt32
				>
			>;

			explicit SecuredString(std::string_view str = {});
			SecuredString(const SecuredString&) = default;
			SecuredString(SecuredString&&) noexcept = default;

			SecuredString& operator=(std::string_view str);
			SecuredString& operator=(const SecuredString&) = default;
			SecuredString& operator=(SecuredString&&) noexcept = default;

			operator std::string&() &;
			operator std::string_view() const&;
			operator std::string&&() &&;

		private:
			std::string m_str;
	};
}

namespace Nz
{
	template<std::size_t N, bool CodepointLimit> bool Serialize(SerializationContext& context, const tsom::SecuredString<N, CodepointLimit>& value, TypeTag<tsom::SecuredString<N>>);
	template<std::size_t N, bool CodepointLimit> bool Unserialize(SerializationContext& context, tsom::SecuredString<N, CodepointLimit>* value, TypeTag<tsom::SecuredString<N>>);
}

#include <CommonLib/Protocol/SecuredString.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_SECUREDSTRING_HPP
