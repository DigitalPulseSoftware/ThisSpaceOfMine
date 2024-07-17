// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_WIDGETSTATE_HPP
#define TSOM_GAME_STATES_WIDGETSTATE_HPP

#include <Game/States/StateData.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace tsom
{
	class StateData;

	class WidgetState : public Nz::State, public std::enable_shared_from_this<WidgetState>
	{
		public:
			WidgetState(std::shared_ptr<StateData> stateData);
			~WidgetState();

		protected:
			template<typename T, typename... Args> void ConnectSignal(T& signal, Args&&... args);
			inline entt::handle CreateEntity();
			template<typename T, typename... Args> T* CreateWidget(Args&&... args);
			inline void DestroyWidget(Nz::BaseWidget* widget);

			inline StateData& GetStateData();
			inline const StateData& GetStateData() const;
			inline std::shared_ptr<StateData>& GetStateDataPtr();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

			virtual void LayoutWidgets(const Nz::Vector2f& newSize);

		private:
			NazaraSlot(Nz::BaseWidget, OnWidgetResized, m_onWidgetResized);

			struct WidgetEntry
			{
				Nz::BaseWidget* widget;
				bool wasVisible = true;
			};

			std::shared_ptr<StateData> m_stateData;
			std::vector<std::function<void()>> m_cleanupFunctions;
			std::vector<WidgetEntry> m_widgets;
			std::vector<entt::handle> m_entities;
			bool m_isVisible;
	};
}

#include <Game/States/WidgetState.inl>

#endif // TSOM_GAME_STATES_WIDGETSTATE_HPP
