// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SESSION_PLAYERSESSIONHANDLER_HPP
#define TSOM_SERVERLIB_SESSION_PLAYERSESSIONHANDLER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <ServerLib/ServerPlayer.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API PlayerSessionHandler : public SessionHandler
	{
		public:
			PlayerSessionHandler(NetworkSession* session, ServerPlayer* player);
			~PlayerSessionHandler();

			void HandlePacket(Packets::MineBlock&& mineBlock);
			void HandlePacket(Packets::PlaceBlock&& placeBlock);
			void HandlePacket(Packets::SendChatMessage&& chatMessage);
			void HandlePacket(Packets::UpdatePlayerInputs&& playerInputs);

			void OnDeserializationError(std::size_t packetIndex) override;
			void OnUnexpectedPacket(std::size_t packetIndex) override;
			void OnUnknownOpcode(Nz::UInt8 opcode) override;

		private:
			bool CheckCanMineBlock(const Chunk* chunk, const Nz::Vector3ui& blockIndices) const;
			bool CheckCanPlaceBlock(const Chunk* chunk, const Nz::Vector3ui& blockIndices) const;

			ServerPlayer* m_player;
	};
}

#include <ServerLib/Session/PlayerSessionHandler.inl>

#endif // TSOM_SERVERLIB_SESSION_PLAYERSESSIONHANDLER_HPP
