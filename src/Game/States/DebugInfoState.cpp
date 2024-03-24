// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/DebugInfoState.hpp>
#include <CommonLib/Version.hpp>
#include <Nazara/TextRenderer/RichTextBuilder.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>

namespace tsom
{
	DebugInfoState::DebugInfoState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData)
	{
		m_fpsEntity = stateData->canvas->Add<Nz::LabelWidget>();

		m_versionEntity = stateData->canvas->Add<Nz::LabelWidget>();
		m_versionEntity->UpdateText(Nz::SimpleTextDrawer::Draw(GetVersionInfo(), 18));
		m_versionEntity->Resize(m_versionEntity->GetPreferredSize());
	}

	void DebugInfoState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		m_fpsCounter = 0;
		m_updateClock.Restart();
	}

	bool DebugInfoState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!WidgetState::Update(fsm, elapsedTime))
			return false;

		m_fpsCounter++;

		if (m_updateClock.RestartIfOver(Nz::Time::Second()))
		{
			m_textDrawer.Clear();

			Nz::RichTextBuilder textBuilder(m_textDrawer);

			textBuilder << Nz::Color::White() << "FPS: ";
			if (m_fpsCounter >= 60)
				textBuilder << Nz::Color::Green();
			else if (m_fpsCounter >= 20)
				textBuilder << Nz::Color::Orange();
			else
				textBuilder << Nz::Color::Red();

			textBuilder << std::to_string(m_fpsCounter);

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
		m_versionEntity->SetPosition({ newSize.x - m_versionEntity->GetWidth(), 0.f });
	}
}
