// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORK_PACKETSERIALIZER_HPP
#define TSOM_COMMONLIB_NETWORK_PACKETSERIALIZER_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <optional>
#include <variant>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API PacketSerializer
	{
		public:
			inline PacketSerializer(Nz::ByteStream& packetBuffer, bool isWriting);
			~PacketSerializer() = default;

			inline Nz::ByteStream& GetStream();

			inline void Read(void* ptr, std::size_t size);

			inline bool IsWriting() const;

			inline void Write(const void* ptr, std::size_t size);

			template<typename DataType> void Serialize(DataType& data);
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
			Nz::ByteStream& m_buffer;
			bool m_isWriting;
	};
}

#include <CommonLib/Protocol/PacketSerializer.inl>

#endif
