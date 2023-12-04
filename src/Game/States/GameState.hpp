// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_GAMESTATE_HPP
#define TSOM_CLIENT_GAMESTATE_HPP

#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <ClientLib/ClientPlanet.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <entt/entt.hpp>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace tsom
{
	class Chatbox;
	struct StateData;

	class GameState : public Nz::State
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
			void OnTick(Nz::Time elapsedTime);
			void SendInputs();
			void UpdateMouseLock();

			NazaraSlot(ClientSessionHandler, OnChatMessage, m_onChatMessage);
			NazaraSlot(ClientSessionHandler, OnChunkCreate, m_onChunkCreate);
			NazaraSlot(ClientSessionHandler, OnChunkDestroy, m_onChunkDestroy);
			NazaraSlot(ClientSessionHandler, OnChunkUpdate, m_onChunkUpdate);
			NazaraSlot(ClientSessionHandler, OnControlledEntityChanged, m_onControlledEntityChanged);
			NazaraSlot(ClientSessionHandler, OnPlayerLeave, m_onPlayerLeave);
			NazaraSlot(ClientSessionHandler, OnPlayerJoined, m_onPlayerJoined);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyPressed, m_onUnhandledKeyPressed);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyReleased, m_onUnhandledKeyReleased);
			NazaraSlot(Nz::WindowEventHandler, OnMouseWheelMoved, m_mouseWheelMovedSlot);

			struct InventorySlot
			{
				std::shared_ptr<Nz::Sprite> sprite;
				entt::handle entity;
			};

			std::shared_ptr<StateData> m_stateData;
			std::size_t m_selectedBlock;
			std::unique_ptr<ClientPlanet> m_planet;
			std::unique_ptr<ClientChunkEntities> m_planetEntities;
			std::unique_ptr<Chatbox> m_chatBox;
			std::vector<InventorySlot> m_inventorySlots;
			entt::handle m_cameraEntity;
			entt::handle m_controlledEntity;
			entt::handle m_sunLightEntity;
			entt::handle m_skyboxEntity;
			Nz::EulerAnglesf m_cameraRotation;
			Nz::Quaternionf m_upCorrection;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			BlockIndex m_selectedBlockIndex;
			EscapeMenu m_escapeMenu;
			bool m_isMouseLocked;
	};
}

#include <Game/States/GameState.inl>

#endif // TSOM_CLIENT_STATES_GAMESTATE_HPP
