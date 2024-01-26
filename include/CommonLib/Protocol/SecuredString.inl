// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/SecuredString.hpp>
#include <Nazara/Core/Stream.hpp>
#include <cassert>

namespace tsom
{
	template<std::size_t N>
	SecuredString<N>::SecuredString(std::string_view str)
	{
	}

	template<std::size_t N>
	SecuredString<N>& SecuredString<N>::operator=(std::string_view str)
	{
		assert(str.size() <= N);
		m_str = str;
		return *this;
	}

	template<std::size_t N>
	SecuredString<N>::operator std::string& () &
	{
		return m_str;
	}

	template<std::size_t N>
	SecuredString<N>::operator std::string_view() const&
	{
		return m_str;
	}

	template<std::size_t N>
	SecuredString<N>::operator std::string&& () &&
	{
		return std::move(m_str);
	}
}

namespace Nz
{
	template<std::size_t N>
	bool Serialize(SerializationContext& context, const tsom::SecuredString<N>& value, TypeTag<tsom::SecuredString<N>>)
	{
		using SizeType = typename tsom::SecuredString<N>::SizeType;

		std::string_view str = value;
		assert(str.size() <= N);

		if (!Serialize(context, SafeCast<SizeType>(str.size()), TypeTag<SizeType>()))
			return false;

		return context.stream->Write(str.data(), str.size()) == str.size();
	}

	template<std::size_t N>
	bool Unserialize(SerializationContext& context, tsom::SecuredString<N>* value, TypeTag<tsom::SecuredString<N>>)
	{
		using SizeType = typename tsom::SecuredString<N>::SizeType;

		SizeType size;
		if (!Unserialize(context, &size, TypeTag<SizeType>()))
			return false;

		if (size > N)
			return false;

		std::string& str = *value;
		str.resize(size);
		return context.stream->Read(&str[0], size) == size;
	}
}
