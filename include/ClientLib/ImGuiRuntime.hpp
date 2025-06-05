// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_IMGUIRUNTIME_HPP
#define TSOM_CLIENTLIB_IMGUIRUNTIME_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>

struct ImGuiContext;

namespace Nz
{
	class ApplicationBase;
	class ImGuiPlugin;
	class Window;
	class WindowSwapchain;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ImGuiRuntime
	{
		public:
			ImGuiRuntime(Nz::ApplicationBase& app, Nz::Window& window, Nz::WindowSwapchain& swapchain);
			ImGuiRuntime(const ImGuiRuntime&) = delete;
			ImGuiRuntime(ImGuiRuntime&&) = delete;
			~ImGuiRuntime();

			void BeginFrame(Nz::Time elapsedTime);
			void EndFrame();

			inline ImGuiContext* GetContext();

			ImGuiRuntime& operator=(const ImGuiRuntime&) = delete;
			ImGuiRuntime& operator=(ImGuiRuntime&&) = delete;

		private:
			Nz::ImGuiPlugin* m_imgui;
			ImGuiContext* m_imguiContext;
	};
}

#include <ClientLib/ImGuiRuntime.inl>

#endif // TSOM_CLIENTLIB_IMGUIRUNTIME_HPP
