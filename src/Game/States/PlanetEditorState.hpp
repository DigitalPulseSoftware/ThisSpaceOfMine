// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
#define TSOM_GAME_STATES_PLANETEDITORSTATE_HPP

#include <ClientLib/ClientChunkEntities.hpp>
#include <CommonLib/ConsoleExecutor.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <memory>
#include <optional>

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
	class SimpleLabelWidget;
	class TextAreaWidget;
}

namespace tsom
{
	class ClientChunkEntities;
	class EscapeMenu;
	class Planet;

	class PlanetEditorState : public WidgetState
	{
		public:
			PlanetEditorState(std::shared_ptr<StateData> stateData);
			PlanetEditorState(const PlanetEditorState&) = delete;
			PlanetEditorState(PlanetEditorState&&) = delete;
			~PlanetEditorState();

			void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

			PlanetEditorState& operator=(const PlanetEditorState&) = delete;
			PlanetEditorState& operator=(PlanetEditorState&&) = delete;

		private:
			void RefreshPlanet();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void UpdateMouseLock();

			NazaraSlot(Nz::Canvas, OnUnhandledKeyPressed, m_onUnhandledKeyPressed);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyReleased, m_onUnhandledKeyReleased);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseButtonPressed, m_mouseButtonReleasedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseMoved, m_mouseMovedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseWheelMoved, m_mouseWheelMovedSlot);

			std::optional<ConsoleExecutor> m_consoleExecutor;
			std::unique_ptr<ClientChunkEntities> m_planetEntities;
			std::unique_ptr<Planet> m_planet;
			entt::handle m_cameraEntity;
			entt::handle m_planetParentEntity;
			entt::handle m_skyboxEntity;
			entt::handle m_sunLightEntity;
			Nz::BoxLayout* m_widgetLayout;
			Nz::TextAreaWidget* m_chunkCountXArea;
			Nz::TextAreaWidget* m_chunkCountYArea;
			Nz::TextAreaWidget* m_chunkCountZArea;
			Nz::TextAreaWidget* m_cornerRadiusArea;
			Nz::TextAreaWidget* m_scriptArea;
			Nz::TextAreaWidget* m_seedArea;
			Nz::EulerAnglesf m_cameraRotation;
			EscapeMenu* m_escapeMenu;
			bool m_isMouseLocked;
			bool m_shouldFreeMouse;
	};
}

#include <Game/States/PlanetEditorState.inl>

#endif // TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
