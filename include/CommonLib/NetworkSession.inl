// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	inline std::size_t NetworkSession::GetPeerId() const
	{
		return m_peerId;
	}

	inline Nz::UInt32 NetworkSession::GetProtocolVersion() const
	{
		return m_protocolVersion;
	}

	inline SessionHandler* NetworkSession::GetSessionHandler()
	{
		return m_sessionHandler.get();
	}

	inline NetworkStringStore& NetworkSession::GetStringStore()
	{
		return m_stringStore;
	}

	inline const NetworkStringStore& NetworkSession::GetStringStore() const
	{
		return m_stringStore;
	}

	inline bool NetworkSession::IsConnected() const
	{
		return m_peerId != NetworkReactor::InvalidPeerId;
	}

	template<typename T>
	void NetworkSession::SendPacket(const T& packet, std::function<void()> acknowledgeCallback)
	{
		static_assert(PacketCount < 0xFF);

		const SessionHandler::SendAttributes& sendAttributes = m_sessionHandler->GetPacketAttributes<T>();

		Nz::ByteArray byteArray;
		Nz::ByteStream byteStream(&byteArray, Nz::OpenMode::Write);
		byteStream << Nz::UInt8(PacketIndex<T>);

		PacketSerializer serializer(byteStream, true, m_protocolVersion);
		Packets::Serialize(serializer, const_cast<T&>(packet));

		byteStream.FlushBits();

		m_reactor.SendData(m_peerId, sendAttributes.channel, sendAttributes.flags, std::move(byteArray), std::move(acknowledgeCallback));
	}

	inline void NetworkSession::SetProtocolVersion(Nz::UInt32 protocolVersion)
	{
		assert(m_protocolVersion == 0);
		m_protocolVersion = protocolVersion;
	}

	template<typename T, typename ...Args>
	T& NetworkSession::SetupHandler(Args&&... args)
	{
		return static_cast<T&>(SetHandler(std::make_unique<T>(this, std::forward<Args>(args)...)));
	}
}
