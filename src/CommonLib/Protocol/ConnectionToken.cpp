// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/ConnectionToken.hpp>
#include <NazaraUtils/Endianness.hpp>
#include <sodium.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <cassert>

namespace tsom
{
	namespace
	{
		template<std::size_t N, std::integral T>
		void SerializeBinary(std::array<Nz::UInt8, N>& buffer, std::size_t& offset, T data)
		{
			data = Nz::HostToLittleEndian(data);
			std::memcpy(&buffer[offset], &data, sizeof(data));
			offset += sizeof(data);
		}

		template<std::size_t N, size_t BufSize>
		void SerializeBinary(std::array<Nz::UInt8, N>& buffer, std::size_t& offset, const std::array<Nz::UInt8, BufSize>& data)
		{
			std::memcpy(&buffer[offset], &data, sizeof(data));
			offset += sizeof(data);
		}
	}

	namespace Packets
	{
		void Serialize(PacketSerializer& serializer, ConnectionToken& token)
		{
			serializer &= token.tokenVersion;
			if (serializer.IsWriting())
				serializer.Write(token.tokenNonce.data(), token.tokenNonce.size());
			else
				serializer.Read(token.tokenNonce.data(), token.tokenNonce.size());

			serializer &= token.creationTimestamp;
			serializer &= token.expireTimestamp;

			if (serializer.IsWriting())
			{
				serializer.Write(token.encryption.clientToServerKey.data(), token.encryption.clientToServerKey.size());
				serializer.Write(token.encryption.serverToClientKey.data(), token.encryption.serverToClientKey.size());
			}
			else
			{
				serializer.Read(token.encryption.clientToServerKey.data(), token.encryption.clientToServerKey.size());
				serializer.Read(token.encryption.serverToClientKey.data(), token.encryption.serverToClientKey.size());
			}

			serializer &= token.gameServer.address;
			serializer &= token.gameServer.port;

			if (serializer.IsWriting())
			{
				Nz::UInt16 privateTokenSize = Nz::SafeCaster(token.privateToken.size());
				serializer &= privateTokenSize;
				serializer.Write(token.privateToken.data(), token.privateToken.size());
			}
			else
			{
				Nz::UInt16 privateTokenSize;
				serializer &= privateTokenSize;

				token.privateToken.resize(privateTokenSize);
				serializer.Read(token.privateToken.data(), token.privateToken.size());
			}
		}

		void Serialize(PacketSerializer& serializer, ConnectionTokenPrivate& token)
		{
			serializer &= token.api.token;
			serializer &= token.api.url;

			serializer &= token.player.uuid;
			serializer &= token.player.nickname;

			if (serializer.GetProtocolVersion() >= 2)
			{
				Nz::UInt32 permissionCount;
				if (serializer.IsWriting())
					permissionCount = Nz::SafeCaster(token.player.permissions.size());
				else
				{
					serializer &= permissionCount;
					token.player.permissions.resize(permissionCount);
				}

				for (std::string& permission : token.player.permissions)
					serializer &= permission;
			}
		}
	}

	Nz::Result<ConnectionToken, std::string> ConnectionToken::Deserialize(const nlohmann::json& doc)
	{
		using base64 = cppcodec::base64_rfc4648;

		auto DecodeBase64Field = []<std::size_t N>(const nlohmann::json& doc, const char* field, std::array<std::uint8_t, N>& bin) -> Nz::Result<void, std::string>
		{
			const std::string& b64 = doc[field];
			std::vector<std::uint8_t> decodedField = base64::decode(b64);
			if (decodedField.size() != bin.size())
				return Nz::Err(fmt::format("{} has wrong size", field));

			std::memcpy(bin.data(), decodedField.data(), decodedField.size());
			return Nz::Ok();
		};

		try
		{
			ConnectionToken connectionToken;
			connectionToken.tokenVersion = doc["token_version"];
			NAZARA_TRY(DecodeBase64Field(doc, "token_nonce", connectionToken.tokenNonce));

			connectionToken.creationTimestamp = Nz::Timestamp::FromSeconds(doc["creation_timestamp"]);
			connectionToken.expireTimestamp = Nz::Timestamp::FromSeconds(doc["expire_timestamp"]);

			const nlohmann::json& encryptionKeys = doc["encryption_keys"];
			NAZARA_TRY(DecodeBase64Field(encryptionKeys, "client_to_server", connectionToken.encryption.clientToServerKey));
			NAZARA_TRY(DecodeBase64Field(encryptionKeys, "server_to_client", connectionToken.encryption.serverToClientKey));

			const nlohmann::json& gameServerAddress = doc["game_server"];
			connectionToken.gameServer.address = gameServerAddress["address"];
			connectionToken.gameServer.port = gameServerAddress["port"];

			const std::string& privateTokenStr = doc["private_token_data"];
			connectionToken.privateToken = base64::decode(privateTokenStr);

			return connectionToken;
		}
		catch (const std::exception& e)
		{
			return Nz::Err(e.what());
		}
	}

	ConnectionTokenAuth ConnectionTokenPrivate::AuthAndDecrypt(const ConnectionToken& connectionToken, std::span<const Nz::UInt8, 32> key, ConnectionTokenPrivate* token)
	{
		assert(token);

		if (Nz::Timestamp::Now() >= connectionToken.expireTimestamp)
			return ConnectionTokenAuth::ExpiredToken;

		std::vector<Nz::UInt8> tokenBinary(ConnectionToken::TokenPrivateMaxSize);
		unsigned long long tokenBinarySize = ConnectionToken::TokenPrivateMaxSize;

		// Validate additional data
		constexpr std::size_t AdditionalSize = sizeof(connectionToken.tokenVersion) + sizeof(connectionToken.expireTimestamp) + sizeof(connectionToken.encryption.clientToServerKey) + sizeof(connectionToken.encryption.serverToClientKey);
		std::array<Nz::UInt8, AdditionalSize> additionalData;
		{
			std::size_t offset = 0;
			SerializeBinary(additionalData, offset, connectionToken.tokenVersion);
			SerializeBinary(additionalData, offset, connectionToken.expireTimestamp.AsSeconds());
			SerializeBinary(additionalData, offset, connectionToken.encryption.clientToServerKey);
			SerializeBinary(additionalData, offset, connectionToken.encryption.serverToClientKey);
		}

		if (crypto_aead_xchacha20poly1305_ietf_decrypt(tokenBinary.data(), &tokenBinarySize, nullptr, connectionToken.privateToken.data(), connectionToken.privateToken.size(), additionalData.data(), additionalData.size(), connectionToken.tokenNonce.data(), key.data()) != 0)
			return ConnectionTokenAuth::ForgedToken;

		try
		{
			Nz::ByteStream byteStream(tokenBinary.data(), tokenBinarySize);
			byteStream.SetDataEndianness(Nz::Endianness::LittleEndian);

			PacketSerializer serializer(byteStream, false, connectionToken.tokenVersion);
			Packets::Serialize(serializer, *token);
		}
		catch (const std::exception& e)
		{
			return ConnectionTokenAuth::CorruptToken;
		}

		return ConnectionTokenAuth::Success;
	}
}
