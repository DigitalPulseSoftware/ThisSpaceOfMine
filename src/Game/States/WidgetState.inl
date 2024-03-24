// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <cassert>

namespace tsom
{
	template<typename T, typename ...Args>
	void WidgetState::ConnectSignal(T& signal, Args&&... args)
	{
		m_cleanupFunctions.emplace_back([connection = signal.Connect(std::forward<Args>(args)...)]() mutable { connection.Disconnect(); });
	}

	template<typename T, typename... Args>
	T* WidgetState::CreateWidget(Args&&... args)
	{
		T* widget = m_stateData->canvas->Add<T>(std::forward<Args>(args)...);

		auto& entry = m_widgets.emplace_back();
		entry.widget = widget;

		if (!m_isVisible)
			entry.widget->Hide();

		return widget;
	}

	inline entt::handle WidgetState::CreateEntity()
	{
		entt::handle entity = m_stateData->world->CreateEntity();
		if (!m_isVisible)
			entity.emplace_or_replace<Nz::DisabledComponent>();

		m_entities.emplace_back(entity);

		return entity;
	}

	inline void WidgetState::DestroyWidget(Nz::BaseWidget* widget)
	{
		auto it = std::find_if(m_widgets.begin(), m_widgets.end(), [&](const WidgetEntry& widgetEntity) { return widgetEntity.widget == widget; });
		assert(it != m_widgets.end());

		m_widgets.erase(it);

		widget->Destroy();
	}

	inline StateData& WidgetState::GetStateData()
	{
		return *m_stateData;
	}

	inline const StateData& WidgetState::GetStateData() const
	{
		return *m_stateData;
	}

	inline std::shared_ptr<StateData>& WidgetState::GetStateDataPtr()
	{
		return m_stateData;
	}
}
