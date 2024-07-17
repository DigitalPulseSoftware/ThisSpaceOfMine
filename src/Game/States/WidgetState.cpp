// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>

namespace tsom
{
	WidgetState::WidgetState(std::shared_ptr<StateData> stateData) :
	m_stateData(std::move(stateData)),
	m_isVisible(false)
	{
		m_onWidgetResized.Connect(m_stateData->canvas->OnWidgetResized, [this](const Nz::BaseWidget*, const Nz::Vector2f& newSize)
		{
			if (m_isVisible)
				LayoutWidgets(newSize);
		});
	}

	WidgetState::~WidgetState()
	{
		for (const auto& cleanupFunc : m_cleanupFunctions)
			cleanupFunc();

		for (WidgetEntry& entry : m_widgets)
			entry.widget->Destroy();

		for (entt::handle& entity : m_entities)
			entity.destroy();
	}

	void WidgetState::Enter(Nz::StateMachine& /*fsm*/)
	{
		m_isVisible = true;

		for (WidgetEntry& entry : m_widgets)
		{
			if (entry.wasVisible)
				entry.widget->Show();
		}

		for (auto it = m_entities.begin(); it != m_entities.end();)
		{
			entt::handle entity = *it;
			if (entity)
			{
				entity.erase<Nz::DisabledComponent>();
				++it;
			}
			else
				it = m_entities.erase(it);
		}

		LayoutWidgets(m_stateData->canvas->GetSize());
	}

	void WidgetState::Leave(Nz::StateMachine& /*fsm*/)
	{
		m_isVisible = false;

		for (WidgetEntry& entry : m_widgets)
		{
			entry.wasVisible = entry.widget->IsVisible();
			entry.widget->Hide();
		}

		for (auto it = m_entities.begin(); it != m_entities.end();)
		{
			entt::handle entity = *it;
			if (entity)
			{
				entity.emplace_or_replace<Nz::DisabledComponent>();
				++it;
			}
			else
				it = m_entities.erase(it);
		}
	}

	bool WidgetState::Update(Nz::StateMachine& /*fsm*/, Nz::Time /*elapsedTime*/)
	{
		return true;
	}

	void WidgetState::LayoutWidgets(const Nz::Vector2f& /*newSize*/)
	{
	}
}
