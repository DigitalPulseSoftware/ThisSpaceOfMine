// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/DebugInfoState.hpp>
#include <ClientLib/ClientConfigs.hpp>
#include <CommonLib/ConfigFile.hpp>
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
			auto& stateData = GetStateData();

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

			Nz::Int16 fpsLimit = stateData.config->GetIntegerValue<Nz::Int16>(Config::Graphics_FPSLimit);
			if (fpsLimit > 0)
				textBuilder << Nz::Color::White() << " (Limit: " << std::to_string(fpsLimit) << ")";

			m_fpsEntity->UpdateText(m_textDrawer);

			LayoutWidgets(stateData.canvas->GetSize());

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
