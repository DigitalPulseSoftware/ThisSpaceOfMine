// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_WIDGETSTATE_HPP
#define TSOM_CLIENT_WIDGETSTATE_HPP

#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <ClientLib/ClientPlanet.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <entt/entt.hpp>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace tsom
{
	struct StateData;

	class GameState : public Nz::State
	{
		public:
			GameState(std::shared_ptr<StateData> stateData);
			~GameState();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void OnTick(Nz::Time elapsedTime);

			void RebuildPlanet();

			void SendInputs();

			NazaraSlot(ClientSessionHandler, OnControlledEntityChanged, m_onControlledEntityChanged);
			NazaraSlot(ClientSessionHandler, OnVoxelGridUpdate, m_onVoxelGridUpdated);

			std::shared_ptr<StateData> m_stateData;
			std::unique_ptr<ClientPlanet> m_planet;
			entt::handle m_cameraEntity;
			entt::handle m_controlledEntity;
			entt::handle m_planetEntity;
			entt::handle m_skyboxEntity;
			Nz::EulerAnglesf m_cameraRotation;
			Nz::Quaternionf m_upCorrection;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
	};
}

#include <Game/States/GameState.inl>

#endif // TSOM_CLIENT_STATES_GAMESTATE_HPP
