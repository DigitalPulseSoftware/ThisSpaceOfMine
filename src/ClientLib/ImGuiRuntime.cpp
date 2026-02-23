// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ImGuiRuntime.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/PluginManagerAppComponent.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/ImGuiPipelinePass.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Renderer/Plugins/ImGuiPlugin.hpp>
#include <imgui.h>

namespace tsom
{
	ImGuiRuntime::ImGuiRuntime(Nz::ApplicationBase& app, Nz::Window& window, Nz::WindowSwapchain& swapchain)
	{
		auto& pluginManager = app.GetComponent<Nz::PluginManagerAppComponent>();
		m_imgui = &pluginManager.Load<Nz::ImGuiPlugin>();

		IMGUI_CHECKVERSION();
		m_imguiContext = ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		m_imgui->SetupContext(m_imguiContext, window);
		m_imgui->SetupRenderer(m_imguiContext, swapchain);

		Nz::Graphics* graphics = Nz::Graphics::Instance();
		Nz::ImGuiPipelinePass::RegisterPass(graphics->GetFramePipelinePassRegistry(), *m_imgui, m_imguiContext);
	}

	ImGuiRuntime::~ImGuiRuntime()
	{
		m_imgui->ShutdownRenderer(m_imguiContext);
		m_imgui->ShutdownContext(m_imguiContext);
		ImGui::DestroyContext(m_imguiContext);
	}

	void ImGuiRuntime::BeginFrame(Nz::Time elapsedTime)
	{
		m_imgui->NewFrame(m_imguiContext, elapsedTime);

		ImGui::NewFrame();
	}

	void ImGuiRuntime::EndFrame()
	{
		ImGui::Render();
	}
}
