// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP
#define TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Protocol/PacketSerializer.hpp>
#include <Nazara/Core/Uuid.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <NazaraUtils/Result.hpp>
#include <nlohmann/json_fwd.hpp>
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

	struct TSOM_COMMONLIB_API ConnectionToken
	{
		Nz::UInt32 tokenVersion;
		std::array<Nz::UInt8, 24> tokenNonce;

		Nz::UInt64 creationTimestamp;
		Nz::UInt64 expireTimestamp;

		struct
		{
			std::array<Nz::UInt8, 32> clientToServerKey;
			std::array<Nz::UInt8, 32> serverToClientKey;
		} encryption;

		struct
		{
			std::string address;
			Nz::UInt64 port;
		} gameServer;

		std::vector<Nz::UInt8> privateToken;

		static constexpr Nz::UInt8 CurrentTokenVersion = 2;
		static constexpr std::size_t TokenPrivateMaxSize = 2048;

		static Nz::Result<ConnectionToken, std::string> Deserialize(const nlohmann::json& doc);
	};

	struct TSOM_COMMONLIB_API ConnectionTokenPrivate
	{
		// API part
		struct
		{
			std::string token;
			std::string url;
		} api;

		// Player info
		struct
		{
			Nz::Uuid uuid;
			std::string nickname;
			std::vector<std::string> permissions;
		} player;

		static ConnectionTokenAuth AuthAndDecrypt(const ConnectionToken& connectionToken, std::span<const Nz::UInt8, 32> key, ConnectionTokenPrivate* token);
	};

	namespace Packets
	{
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ConnectionToken& token);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ConnectionTokenPrivate& token);
	}
}

#include <CommonLib/Protocol/ConnectionToken.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_CONNECTIONTOKEN_HPP
