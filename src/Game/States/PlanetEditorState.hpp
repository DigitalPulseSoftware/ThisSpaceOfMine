// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
#define TSOM_GAME_STATES_PLANETEDITORSTATE_HPP

#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/ChunkGenerator.hpp>
#include <CommonLib/ConsoleExecutor.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/State.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <memory>
#include <optional>
#include <unordered_map>

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
	class EntityClass;
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
			void RefreshScript();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void LoadScripts();
			void UpdateMouseLock();

			NazaraSlot(Nz::Canvas, OnUnhandledKeyPressed, m_onUnhandledKeyPressed);
			NazaraSlot(Nz::Canvas, OnUnhandledKeyReleased, m_onUnhandledKeyReleased);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseButtonPressed, m_mouseButtonReleasedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseMoved, m_mouseMovedSlot);
			NazaraSlot(Nz::Canvas, OnUnhandledMouseWheelMoved, m_mouseWheelMovedSlot);

			struct PlanetSettings
			{
				Nz::Vector3ui chunkCount = Nz::Vector3ui(10);
				std::string scriptName = "bob";
				std::unordered_map<std::string, EntityProperty> properties;
			};

			std::array<std::unique_ptr<ClientChunkEntities>, Constants::MaxChunkLayerCount> m_planetEntities;
			std::optional<ConsoleExecutor> m_consoleExecutor;
			std::shared_ptr<const EntityClass> m_planetClass;
			std::unique_ptr<Planet> m_planet;
			entt::handle m_atmosphereEntity;
			entt::handle m_cameraEntity;
			entt::handle m_planetParentEntity;
			entt::handle m_skyboxEntity;
			entt::handle m_sunLightEntity;
			Nz::EulerAnglesf m_cameraRotation;
			AtmosphereScattering m_atmosphereSettings;
			ChunkGenerator m_chunkGenerator;
			EntityRegistry m_entityRegistry;
			PlanetSettings m_planetSettings;
			EscapeMenu* m_escapeMenu;
			ImGuiContext* m_imguiContext;
			bool m_isMouseLocked;
			bool m_lockInputs;
	};
}

#include <Game/States/PlanetEditorState.inl>

#endif // TSOM_GAME_STATES_PLANETEDITORSTATE_HPP
