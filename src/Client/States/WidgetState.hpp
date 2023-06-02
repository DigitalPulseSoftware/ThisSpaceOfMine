// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_WIDGETSTATE_HPP
#define TSOM_CLIENT_STATES_WIDGETSTATE_HPP

#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Renderer/RenderTarget.hpp>
#include <Nazara/Widgets/BaseWidget.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace tsom
{
	class WidgetState : public Nz::State
	{
		public:
			WidgetState(Nz::BaseWidget* parentWidget, Nz::EnttWorld& world);
			~WidgetState();

		protected:
			template<typename T, typename... Args> void ConnectSignal(T& signal, Args&&... args);
			inline entt::handle CreateEntity();
			template<typename T, typename... Args> T* CreateWidget(Args&&... args);
			inline void DestroyWidget(Nz::BaseWidget* widget);

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

			std::vector<std::function<void()>> m_cleanupFunctions;
			std::vector<WidgetEntry> m_widgets;
			std::vector<entt::handle> m_entities;
			Nz::BaseWidget* m_parentWidget;
			Nz::EnttWorld& m_world;
			bool m_isVisible;
	};
}

#include <Client/States/WidgetState.inl>

#endif // TSOM_CLIENT_STATES_WIDGETSTATE_HPP
