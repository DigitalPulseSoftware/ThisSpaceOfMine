// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_GAMESTATE_HPP
#define TSOM_GAME_STATES_GAMESTATE_HPP

#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/ClientPlanet.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <entt/entt.hpp>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Nz
{
	class LabelWidget;
}

namespace tsom
{
	class BlockSelectionBar;
	class Chatbox;
	class EscapeMenu;
	struct StateData;

	class GameState : public WidgetState
	{
		public:
			GameState(std::shared_ptr<StateData> stateData);
			GameState(const GameState&) = delete;
			GameState(GameState&&) = delete;
			~GameState();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

			GameState& operator=(const GameState&) = delete;
			GameState& operator=(GameState&&) = delete;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnTick(Nz::Time elapsedTime, bool lastTick);
			void SendInputs();
			void UpdateMouseLock();

			NazaraSlot(ClientSessionHandler, OnChatMessage, m_onChatMessage);
			NazaraSlot(ClientSessionHandler, OnChunkCreate, m_onChunkCreate);
			NazaraSlot(ClientSessionHandler, OnChunkDestroy, m_onChunkDestroy);
			NazaraSlot(ClientSessionHandler, OnChunkReset, m_onChunkReset);
			NazaraSlot(ClientSessionHandler, OnChunkUpdate, m_onChunkUpdate);
			NazaraSlot(ClientSessionHandler, OnControlledEntityChanged, m_onControlledEntityChanged);
			NazaraSlot(ClientSessionHandler, OnControlledEntityStateUpdate, m_onControlledEntityStateUpdate);
			NazaraSlot(ClientSessionHandler, OnPlayerLeave, m_onPlayerLeave);
			NazaraSlot(ClientSessionHandler, OnPlayerJoined, m_onPlayerJoined);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyPressed, m_onUnhandledKeyPressed);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyReleased, m_onUnhandledKeyReleased);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseButtonPressed, m_mouseButtonReleasedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseMoved, m_mouseMovedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseWheelMoved, m_mouseWheelMovedSlot);

			struct DebugOverlay
			{
				Nz::LabelWidget* label = nullptr;
				Nz::SimpleTextDrawer textDrawer;
				NetworkReactor::PeerInfo peerInfo;
				unsigned int mode = 0;
			};

			struct InputRotation
			{
				InputIndex inputIndex;
				Nz::EulerAnglesf inputRotation;
			};

			std::shared_ptr<DebugOverlay> m_debugOverlay;
			std::unique_ptr<ClientPlanet> m_planet;
			std::unique_ptr<ClientChunkEntities> m_planetEntities;
			std::vector<InputRotation> m_predictedInputRotations;
			entt::handle m_cameraEntity;
			entt::handle m_controlledEntity;
			entt::handle m_sunLightEntity;
			entt::handle m_skyboxEntity;
			Nz::EulerAnglesf m_incomingCameraRotation;  //< Accumulated rotation from inputs (will be applied on inputs)
			Nz::EulerAnglesf m_predictedCameraRotation; //< Rotation sent to the server
			Nz::EulerAnglesf m_remainingCameraRotation; //< Remaining rotation to send to the server (in case we rotate too fast)
			Nz::Quaternionf m_referenceRotation;
			Nz::Quaternionf m_upCorrection;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			Nz::UInt8 m_nextInputIndex;
			BlockSelectionBar* m_blockSelectionBar;
			Chatbox* m_chatBox;
			EscapeMenu* m_escapeMenu;
			bool m_isCameraThirdPerson;
			bool m_isMouseLocked;
	};
}

#include <Game/States/GameState.inl>

#endif // TSOM_GAME_STATES_GAMESTATE_HPP
