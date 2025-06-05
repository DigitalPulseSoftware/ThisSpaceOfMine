// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
#define TSOM_GAME_STATES_PLANETEDITORSTATE_HPP

#include <ClientLib/ClientChunkEntities.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/ConsoleExecutor.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <memory>
#include <optional>

struct ImGuiContext;

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
	class ImGuiPlugin;
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

			struct PlanetSettings
			{
				float cornerRadius = 0.f;
				Nz::Vector3ui chunkCount = Nz::Vector3ui(5);
				std::size_t seed = 42;
				std::string scriptName = "bob";
			};

			std::array<std::unique_ptr<ClientChunkEntities>, Constants::MaxChunkLayerCount> m_planetEntities;
			std::optional<ConsoleExecutor> m_consoleExecutor;
			std::unique_ptr<Planet> m_planet;
			entt::handle m_atmosphereEntity;
			entt::handle m_cameraEntity;
			entt::handle m_planetParentEntity;
			entt::handle m_skyboxEntity;
			entt::handle m_sunLightEntity;
			Nz::EulerAnglesf m_cameraRotation;
			AtmosphereScattering m_atmosphereSettings;
			PlanetSettings m_planetSettings;
			EscapeMenu* m_escapeMenu;
			ImGuiContext* m_imguiContext;
			bool m_isMouseLocked;
			bool m_lockInputs;
	};
}

#include <Game/States/PlanetEditorState.inl>

#endif // TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
