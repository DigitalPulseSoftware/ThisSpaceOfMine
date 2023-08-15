// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/DebugInfoState.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>

namespace tsom
{
	DebugInfoState::DebugInfoState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData)
	{
		m_fpsEntity = stateData->canvas->Add<Nz::LabelWidget>();
	}

	void DebugInfoState::Enter(Nz::StateMachine& /*fsm*/)
	{
		m_fpsCounter = 0;
		m_updateClock.Restart();
	}

	bool DebugInfoState::Update(Nz::StateMachine& /*fsm*/, Nz::Time /*elapsedTime*/)
	{
		m_fpsCounter++;

		if (m_updateClock.RestartIfOver(Nz::Time::Second()))
		{
			m_textDrawer.Clear();
			m_textDrawer.SetDefaultColor(Nz::Color::White());
			m_textDrawer.AppendText("FPS: ");
			if (m_fpsCounter >= 60)
				m_textDrawer.SetDefaultColor(Nz::Color::Green());
			else if (m_fpsCounter >= 20)
				m_textDrawer.SetDefaultColor(Nz::Color::Orange());
			else
				m_textDrawer.SetDefaultColor(Nz::Color::Red());

			m_textDrawer.AppendText(std::to_string(m_fpsCounter));

			m_fpsEntity->UpdateText(m_textDrawer);
			m_fpsEntity->Resize(m_fpsEntity->GetPreferredSize());

			LayoutWidgets(GetStateData().canvas->GetSize());

			m_fpsCounter = 0;
		}

		return true;
	}

	void DebugInfoState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_fpsEntity->SetPosition(newSize - Nz::Vector2f(m_fpsEntity->GetSize()));
	}
}
