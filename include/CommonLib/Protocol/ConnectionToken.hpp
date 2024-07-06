// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP
#define TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP

#include <Nazara/Network/IpAddress.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <CommonLib/Protocol/PacketSerializer.hpp>
#include <array>
#include <span>
#include <string>

namespace tsom
{
	enum class ConnectionTokenAuth
	{
		CorruptToken,
		ForgedToken,
		Success
		// TODO: Add timestamp check
	};

	struct ConnectionToken
	{
		Nz::UInt32 tokenVersion;
		std::array<Nz::UInt8, 12> tokenNonce;

		Nz::UInt64 creationTimestamp;
		Nz::UInt64 expireTimestamp;

		struct
		{
			std::array<Nz::UInt8, 32> keyClientToServer;
			std::array<Nz::UInt8, 32> keyServerToClient;
		} encryption;

		struct
		{
			std::string address;
			Nz::UInt64 port;
		} gameServer;

		std::vector<Nz::UInt8> privateToken;

		static constexpr Nz::UInt8 CurrentTokenVersion = 1;
		static constexpr std::size_t TokenPrivateMaxSize = 2048;
	};

	struct ConnectionTokenPrivate
	{
		// Encryption confirmation
		struct
		{
			std::array<Nz::UInt8, 32> keyClientToServer;
			std::array<Nz::UInt8, 32> keyServerToClient;
		} encryption;

		// API part
		struct
		{
			std::string token;
			std::string url;
		} api;

		// Player info
		struct
		{
			std::string guid;
			std::string nickname;
		} player;

		static ConnectionTokenAuth AuthAndDecrypt(std::span<const Nz::UInt8> tokenData, Nz::UInt32 tokenVersion, Nz::UInt64 expireTimestamp, std::span<const Nz::UInt8, 12> nonce, std::span<const Nz::UInt8, 32> key, ConnectionTokenPrivate* token);
	};

	namespace Packets
	{
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ConnectionToken& token);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ConnectionTokenPrivate& token);
	}
}

#include <CommonLib/Protocol/ConnectionToken.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP
