// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/ConnectionToken.hpp>
#include <NazaraUtils/Endianness.hpp>
#include <sodium.h>
#include <cassert>

namespace tsom
{
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
				serializer.Write(token.encryption.keyClientToServer.data(), token.encryption.keyClientToServer.size());
				serializer.Write(token.encryption.keyServerToClient.data(), token.encryption.keyServerToClient.size());
			}
			else
			{
				serializer.Read(token.encryption.keyClientToServer.data(), token.encryption.keyClientToServer.size());
				serializer.Read(token.encryption.keyServerToClient.data(), token.encryption.keyServerToClient.size());
			}

			serializer &= token.gameServer.address;
			serializer &= token.gameServer.port;

			if (serializer.IsWriting())
				serializer.Write(token.privateToken.data(), token.privateToken.size());
			else
				serializer.Read(token.privateToken.data(), token.privateToken.size());
		}

		void Serialize(PacketSerializer& serializer, ConnectionTokenPrivate& token)
		{
			if (serializer.IsWriting())
			{
				serializer.Write(token.encryption.keyClientToServer.data(), token.encryption.keyClientToServer.size());
				serializer.Write(token.encryption.keyServerToClient.data(), token.encryption.keyServerToClient.size());
			}
			else
			{
				serializer.Read(token.encryption.keyClientToServer.data(), token.encryption.keyClientToServer.size());
				serializer.Read(token.encryption.keyServerToClient.data(), token.encryption.keyServerToClient.size());
			}

			serializer &= token.api.url;
			serializer &= token.api.token;

			serializer &= token.player.guid;
			serializer &= token.player.nickname;
		}
	}

	ConnectionTokenAuth ConnectionTokenPrivate::AuthAndDecrypt(std::span<const Nz::UInt8> tokenData, Nz::UInt32 tokenVersion, Nz::UInt64 expireTimestamp, std::span<const Nz::UInt8, 12> nonce, std::span<const Nz::UInt8, 32> key, ConnectionTokenPrivate* token)
	{
		assert(token);

		std::vector<Nz::UInt8> tokenBinary(ConnectionToken::TokenPrivateMaxSize);
		unsigned long long tokenBinarySize = ConnectionToken::TokenPrivateMaxSize;

		// Serialize additional data
		std::array<Nz::UInt8, sizeof(tokenVersion) + sizeof(expireTimestamp)> additionalData;
		{
			Nz::UInt32 tokenVersionLE = Nz::HostToLittleEndian(tokenVersion);
			Nz::UInt64 expireTimestampLE = Nz::HostToLittleEndian(expireTimestamp);

			std::memcpy(&additionalData[0], &tokenVersionLE, sizeof(tokenVersionLE));
			std::memcpy(&additionalData[sizeof(tokenVersionLE)], &expireTimestampLE, sizeof(expireTimestampLE));
		}

		if (crypto_aead_xchacha20poly1305_ietf_decrypt(tokenBinary.data(), &tokenBinarySize, nullptr, tokenData.data(), tokenData.size(), additionalData.data(), additionalData.size(), nonce.data(), key.data()) != 0)
			return ConnectionTokenAuth::ForgedToken;

		try
		{
			Nz::ByteStream byteStream(tokenBinary.data(), tokenBinarySize);
			PacketSerializer serializer(byteStream, false, tokenVersion);
			Packets::Serialize(serializer, *token);
		}
		catch (const std::exception& e)
		{
			return ConnectionTokenAuth::CorruptToken;
		}

		return ConnectionTokenAuth::Success;
	}
}
