// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Version.hpp> //!< Remove on 0.4.0
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

		// 0.3.2 extended from 2 to 3 channels but 0.3.1 clients can't receive packets on channel 2
		//! Remove on 0.4
		Nz::UInt8 channel = sendAttributes.channel;
		if (m_protocolVersion < BuildVersion(0, 3, 2) && channel >= 2)
			channel = 1;
		//! Remove on 0.4

		m_reactor.SendData(m_peerId, channel, sendAttributes.flags, std::move(byteArray), std::move(acknowledgeCallback));
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
