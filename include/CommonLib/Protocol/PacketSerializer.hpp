// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_PACKETSERIALIZER_HPP
#define TSOM_COMMONLIB_PROTOCOL_PACKETSERIALIZER_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <NazaraUtils/Result.hpp>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace tsom
{
	template<typename T> concept EnumType = std::is_enum_v<T>;

	class TSOM_COMMONLIB_API PacketSerializer
	{
		public:
			inline PacketSerializer(Nz::ByteStream& packetStream, bool isWriting, Nz::UInt32 protocolVersion);
			~PacketSerializer() = default;

			inline Nz::ByteStream& GetByteStream();
			inline Nz::UInt32 GetProtocolVersion() const;

			inline void Read(void* ptr, std::size_t size);

			inline bool IsWriting() const;

			inline void Write(const void* ptr, std::size_t size);

			template<typename DataType> void Serialize(DataType& data);
			template<EnumType E> void Serialize(E& data);
			template<typename Value, typename Error> void Serialize(Nz::Result<Value, Error>& result);
			template<typename Error> void Serialize(Nz::Result<void, Error>& result);
			template<typename DataType> void Serialize(std::optional<DataType>& opt);
			template<typename F, typename... Types> void Serialize(std::variant<Types...>& variant, F&& functor);
			template<typename DataType> void Serialize(std::vector<DataType>& dataVec);
			template<typename DataType> void Serialize(const DataType& data) const;
			template<typename PacketType, typename DataType> void Serialize(DataType& data);
			template<typename PacketType, typename DataType> void Serialize(const DataType& data) const;

			template<typename T> void SerializeArraySize(T& array);
			template<typename T> void SerializeArraySize(const T& array);

			template<typename DataType> void SerializePresence(std::optional<DataType>& dataOpt);

			template<typename E, typename UT = std::underlying_type_t<E>> void SerializeEnum(E& enumValue);

			template<typename DataType> void operator&=(DataType& data);
			template<typename DataType> void operator&=(const DataType& data) const;

		private:
			Nz::ByteStream& m_stream;
			Nz::UInt32 m_protocolVersion;
			bool m_isWriting;
	};
}

#include <CommonLib/Protocol/PacketSerializer.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_PACKETSERIALIZER_HPP
