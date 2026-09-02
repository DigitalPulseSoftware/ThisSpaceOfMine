// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_CAMERATOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_CAMERATOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <Nazara/Math/Angle.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <entt/entt.hpp>
#include <functional>

namespace Nz
{
	class EnttWorld;
	class TaskScheduler;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API CameraTool final : public ToolBase
	{
		public:
			CameraTool(GameInterface& gameInterface, std::function<void()> exitCallback);

			void OnActivate() override;
			void OnDeactivate() override;

			void OnMouseMoved(float deltaX, float deltaY) override;
			void OnTrigger(TriggerType triggerType) override;

			void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast) override;

			static void TakeCubemapScreenshot(Nz::TaskScheduler& taskScheduler, Nz::EnttWorld& world, entt::handle referenceCamera, Nz::UInt32 size);
			static void TakeScreenshot(Nz::TaskScheduler& taskScheduler, Nz::EnttWorld& world, entt::handle referenceCamera, const Nz::Vector2ui32& size);

		private:
			std::function<void()> m_exitCallback;
			entt::handle m_cameraEntity;
			Nz::DegreeAnglef m_cameraFOV;
			Nz::EulerAnglesf m_cameraRotation;
			Nz::Quaternionf m_initialCameraRotation;
			Nz::Vector2ui32 m_screenshotSize;
			bool m_screenshotCubemap;
	};
}

#include <ClientLib/Tools/CameraTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_CAMERATOOL_HPP
