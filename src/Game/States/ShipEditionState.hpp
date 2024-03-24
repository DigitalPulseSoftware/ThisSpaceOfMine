// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_SHIPEDITIONSTATE_HPP
#define TSOM_GAME_STATES_SHIPEDITIONSTATE_HPP

#include <ClientLib/ClientChunkEntities.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Ship.hpp>
#include <Game/States/WidgetState.hpp>
#include <mutex>
#include <thread>

namespace Nz
{
	class AbstractTextDrawer;
	class BoxLayout;
	class LabelWidget;
	class ScrollAreaWidget;
}

namespace tsom
{
	class ShipEditionState : public WidgetState
	{
		public:
			ShipEditionState(std::shared_ptr<StateData> stateDataPtr);
			ShipEditionState(const ShipEditionState&) = delete;
			ShipEditionState(ShipEditionState&&) = delete;
			~ShipEditionState();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

			ShipEditionState& operator=(const ShipEditionState&) = delete;
			ShipEditionState& operator=(ShipEditionState&&) = delete;

		private:
			struct Area;

			Area BuildArea(std::size_t firstBlockIndex, Nz::Bitset<Nz::UInt64>& remainingBlocks) const;
			void CheckHullIntegrity();
			void DrawHoveredFace();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void UpdateStatus(const Nz::AbstractTextDrawer& textDrawer);

			struct Area
			{
				Nz::Bitset<Nz::UInt64> blocks;
			};

			BlockIndex m_currentBlock = EmptyBlockIndex;
			entt::handle m_cameraEntity;
			entt::handle m_skyboxEntity;
			entt::handle m_sunLightEntity;
			std::mutex m_integrityMutex;
			std::thread m_integrityThread;
			std::vector<Area> m_shipAreas;
			std::unique_ptr<Ship> m_ship;
			std::unique_ptr<ClientChunkEntities> m_shipEntities;
			Nz::BoxLayout* m_blockSelectionWidget;
			Nz::LabelWidget* m_infoLabel;
			bool m_cameraMovement;
	};
}

#include <Game/States/ShipEditionState.inl>

#endif // TSOM_GAME_STATES_SHIPEDITIONSTATE_HPP
