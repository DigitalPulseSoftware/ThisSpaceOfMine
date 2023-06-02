// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <cassert>
#include <Nazara/Widgets/Debug.hpp>

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
		T* widget = m_parentWidget->Add<T>(std::forward<Args>(args)...);

		auto& entry = m_widgets.emplace_back();
		entry.widget = widget;

		if (!m_isVisible)
			entry.widget->Hide();

		return widget;
	}

	inline entt::handle WidgetState::CreateEntity()
	{
		entt::handle entity = m_world.CreateEntity();
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
}

#include <Nazara/Widgets/DebugOff.hpp>
