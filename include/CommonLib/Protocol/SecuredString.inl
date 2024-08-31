// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/SecuredString.hpp>
#include <Nazara/Core/Stream.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <cassert>

namespace tsom
{
	template<std::size_t N, bool CodepointLimit>
	SecuredString<N, CodepointLimit>::SecuredString(std::string_view str) :
	m_str(str)
	{
		if constexpr (CodepointLimit)
			assert(Nz::ComputeCharacterCount(str) <= N);
		else
			assert(str.size() <= N);
	}

	template<std::size_t N, bool CodepointLimit>
	SecuredString<N, CodepointLimit>& SecuredString<N, CodepointLimit>::operator=(std::string_view str)
	{
		if constexpr (CodepointLimit)
			assert(Nz::ComputeCharacterCount(str) <= N);
		else
			assert(str.size() <= N);

		m_str = str;
		return *this;
	}

	template<std::size_t N, bool CodepointLimit>
	std::string& SecuredString<N, CodepointLimit>::Str() &
	{
		return m_str;
	}

	template<std::size_t N, bool CodepointLimit>
	const std::string& SecuredString<N, CodepointLimit>::Str() const&
	{
		return m_str;
	}

	template<std::size_t N, bool CodepointLimit>
	std::string&& SecuredString<N, CodepointLimit>::Str() &&
	{
		return std::move(m_str);
	}

	template<std::size_t N, bool CodepointLimit>
	SecuredString<N, CodepointLimit>::operator std::string& () &
	{
		return m_str;
	}

	template<std::size_t N, bool CodepointLimit>
	SecuredString<N, CodepointLimit>::operator std::string_view() const&
	{
		return m_str;
	}

	template<std::size_t N, bool CodepointLimit>
	SecuredString<N, CodepointLimit>::operator std::string&& () &&
	{
		return std::move(m_str);
	}
}

namespace Nz
{
	template<std::size_t N, bool CodepointLimit>
	bool Serialize(SerializationContext& context, const tsom::SecuredString<N, CodepointLimit>& value, TypeTag<tsom::SecuredString<N>>)
	{
		using SizeType = typename tsom::SecuredString<N>::SizeType;

		std::string_view str = value;
		if constexpr (CodepointLimit)
			assert(Nz::ComputeCharacterCount(str) <= N);
		else
			assert(str.size() <= N);

		if (!Serialize(context, SafeCast<SizeType>(str.size()), TypeTag<SizeType>()))
			return false;

		return context.stream->Write(str.data(), str.size()) == str.size();
	}

	template<std::size_t N, bool CodepointLimit>
	bool Deserialize(SerializationContext& context, tsom::SecuredString<N, CodepointLimit>* value, TypeTag<tsom::SecuredString<N>>)
	{
		using SizeType = typename tsom::SecuredString<N>::SizeType;

		SizeType size;
		if (!Deserialize(context, &size, TypeTag<SizeType>()))
			return false;

		if constexpr (CodepointLimit)
		{
			// Each codepoint can take up to 4 bytes/char
			if (size > N * 4)
				return false;
		}
		else
		{
			if (size > N)
				return false;
		}

		std::string& str = *value;
		str.resize(size);
		if (context.stream->Read(&str[0], size) != size)
			return false;

		if constexpr (CodepointLimit)
		{
			if (Nz::ComputeCharacterCount(str) > N)
				return false;
		}

		return true;
	}
}
