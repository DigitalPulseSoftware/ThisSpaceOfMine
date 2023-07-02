// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/ConnectionState.hpp>
#include <Game/States/BackgroundState.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <fmt/format.h>

namespace tsom
{
	ConnectionState::ConnectionState(std::shared_ptr<StateData> stateData) :
	WidgetState(std::move(stateData)),
	m_reactor(0, Nz::NetProtocol::Any, 0, 2),
	m_connectingLabel(nullptr)
	{
		m_connectingLabel = CreateWidget<Nz::LabelWidget>();
	}

	void ConnectionState::Connect(const Nz::IpAddress& serverAddress, std::shared_ptr<Nz::State> previousState, std::shared_ptr<Nz::State> nextState)
	{
		Disconnect();

		m_previousState = std::move(previousState);
		m_connectedState = std::move(nextState);

		std::size_t peerId = m_reactor.ConnectTo(serverAddress);
		m_serverSession.emplace(m_reactor, peerId, serverAddress);
		m_serverSession->SetupHandler<ClientSessionHandler>();

		GetStateData().networkSession = &m_serverSession.value();

		m_connectingLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Connecting to " + serverAddress.ToString() + "...", 48));
		m_connectingLabel->Center();
	}

	void ConnectionState::Disconnect()
	{
		if (m_serverSession)
		{
			m_serverSession->Disconnect();
			GetStateData().networkSession = nullptr;
		}

		m_connectingLabel->Hide();
	}

	void ConnectionState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_connectingLabel->Center();
	}

	bool ConnectionState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		auto ConnectionHandler = [&]([[maybe_unused]] bool outgoingConnection, std::size_t peerIndex, const Nz::IpAddress& remoteAddress, [[maybe_unused]] Nz::UInt32 data)
		{
			if (!m_serverSession || m_serverSession->GetPeerId() != peerIndex)
				return;

			m_connectingLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Authenticating...", 48));
			m_connectingLabel->Center();

			Packets::AuthRequest request;
			request.nickname = "SirLynix";

			m_serverSession->SendPacket(request);

			m_nextState = m_connectedState;
			m_nextStateTimer = Nz::Time::Milliseconds(500);
		};

		auto DisconnectionHandler = [&](std::size_t peerIndex, [[maybe_unused]] Nz::UInt32 data)
		{
			if (!m_serverSession || m_serverSession->GetPeerId() != peerIndex)
				return;

			m_connectingLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Connection lost.", 48, Nz::TextStyle_Regular, Nz::Color::Red()));
			m_connectingLabel->Center();

			fmt::print("Disconnected from server\n");

			m_nextState = m_previousState;
			m_nextStateTimer = Nz::Time::Milliseconds(2000);

			m_serverSession.reset();
		};

		auto PacketHandler = [&](std::size_t peerIndex, Nz::NetPacket&& packet)
		{
			if NAZARA_UNLIKELY(!m_serverSession || m_serverSession->GetPeerId() != peerIndex)
				return;

			m_serverSession->HandlePacket(std::move(packet));
		};

		m_reactor.Poll(ConnectionHandler, DisconnectionHandler, PacketHandler);

		if (m_nextState)
		{
			m_nextStateTimer -= elapsedTime;
			if (m_nextStateTimer <= Nz::Time::Zero())
			{
				fsm.PopStatesUntil(shared_from_this());
				if (m_nextState == m_previousState)
					fsm.PushState(std::make_shared<BackgroundState>(GetStateDataPtr()));
				fsm.PushState(m_nextState);
				m_nextState = nullptr;

				m_connectingLabel->Hide();
			}
		}

		return true;
	}
}
