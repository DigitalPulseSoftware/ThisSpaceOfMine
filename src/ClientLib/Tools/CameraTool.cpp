// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/CameraTool.hpp>
#include <ClientLib/ClientConfigs.hpp>
#include <ClientLib/GameInterface.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/ConfigFile.hpp>
#include <CommonLib/Direction.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderTexture.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Platform/Keyboard.hpp>
#include <spdlog/spdlog.h>

#ifdef TSOM_DEV_TOOLS
#include <imgui.h>
#endif

namespace tsom
{
	CameraTool::CameraTool(GameInterface& gameInterface, std::function<void()> exitCallback) :
	ToolBase(gameInterface, "Camera tool"),
	m_exitCallback(std::move(exitCallback)),
	m_cameraRotation(Nz::EulerAnglesf::Zero()),
	m_screenshotCubemap(false)
	{
		auto& config = m_gameInterface.GetConfig();
		m_screenshotSize.x = config.GetIntegerValue<Nz::UInt32>(Config::Screenshot_Width);
		m_screenshotSize.y = config.GetIntegerValue<Nz::UInt32>(Config::Screenshot_Height);
	}

	void CameraTool::OnActivate()
	{
		entt::handle camera2D = m_gameInterface.GetCamera2DEntity();
		entt::handle camera3D = m_gameInterface.GetCamera3DEntity();

		camera2D.emplace<Nz::DisabledComponent>();
		camera3D.emplace<Nz::DisabledComponent>();

		m_cameraEntity = m_gameInterface.GetWorld().CreateEntity();
		m_cameraEntity.emplace<Nz::NodeComponent>(camera3D.get<Nz::NodeComponent>());
		m_cameraEntity.emplace<AtmosphereScatteringCameraSettings>(camera3D.get<AtmosphereScatteringCameraSettings>());

		auto& camera = m_cameraEntity.emplace<Nz::CameraComponent>(camera3D.get<Nz::CameraComponent>());
		camera.UpdateRenderMask(Constants::RenderMask3D);

		m_cameraFOV = camera.GetFOV();
		m_cameraRotation = Nz::EulerAnglesf::Zero();
	}

	void CameraTool::OnDeactivate()
	{
		entt::handle camera2D = m_gameInterface.GetCamera2DEntity();
		entt::handle camera3D = m_gameInterface.GetCamera3DEntity();
		entt::handle controlledEntity = m_gameInterface.GetControlledEntity();

		camera2D.erase<Nz::DisabledComponent>();
		camera3D.erase<Nz::DisabledComponent>();
		controlledEntity.remove<Nz::DisabledComponent>();

		m_cameraEntity.destroy();
	}

	void CameraTool::OnMouseMoved(float deltaX, float deltaY)
	{
		auto& config = m_gameInterface.GetConfig();
		float mouseSensitivity = config.GetFloatValue<float>(Config::Input_MouseSensitivity);

		m_cameraRotation.pitch -= deltaY * mouseSensitivity;
		m_cameraRotation.yaw -= deltaX * mouseSensitivity;
	}

	void CameraTool::OnTrigger(TriggerType triggerType)
	{
		if (triggerType != TriggerType::Primary && triggerType != TriggerType::Secondary)
			return;

		if (triggerType == ToolBase::TriggerType::Secondary)
		{
			m_isCursorUnlocked = !m_isCursorUnlocked;
			m_gameInterface.UpdateMouseLock();
		}
	}

	void CameraTool::Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* /*previewRaycast*/)
	{
		float cameraSpeed = 5.f * elapsedTime.AsSeconds();
		float cameraRotationSpeed = 90.f * elapsedTime.AsSeconds();
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift))
			cameraSpeed *= 10.f;

		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		Nz::Vector3f cameraPos = cameraNode.GetPosition();
		Nz::Quaternionf cameraRot = cameraNode.GetRotation();

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W))
			cameraPos += cameraNode.GetForward() * cameraSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S))
			cameraPos += cameraNode.GetBackward() * cameraSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A))
			cameraPos += cameraNode.GetLeft() * cameraSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D))
			cameraPos += cameraNode.GetRight() * cameraSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::Q))
			m_cameraRotation.roll -= cameraRotationSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::E))
			m_cameraRotation.roll += cameraRotationSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::Space))
			cameraPos += Nz::Vector3f::Up() * cameraSpeed;

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LControl))
			cameraPos += Nz::Vector3f::Down() * cameraSpeed;

		cameraRot = Nz::Quaternionf::CombineRotations(cameraRot, Nz::Quaternionf(Nz::DegreeAnglef(m_cameraRotation.yaw), cameraNode.GetUp()));
		cameraRot = Nz::Quaternionf::CombineRotations(cameraRot, Nz::Quaternionf(Nz::DegreeAnglef(m_cameraRotation.pitch), cameraNode.GetRight()));
		cameraRot = Nz::Quaternionf::CombineRotations(cameraRot, Nz::Quaternionf(Nz::DegreeAnglef(m_cameraRotation.roll), cameraNode.GetForward()));
		cameraRot.Normalize();

		m_cameraRotation = Nz::EulerAnglesf::Zero();

#if defined(TSOM_DEV_TOOLS)
		auto& camera = m_cameraEntity.get<Nz::CameraComponent>();

		if (ImGui::Begin("Camera mode", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Press right click to lock/unlock mouse");
			ImGui::Separator();

			if (ImGui::Button("Close"))
			{
				m_exitCallback(); //< Instance will be invalided after this call
				ImGui::End();
				return;
			}

			if (ImGui::DragFloat("Camera FOV", &m_cameraFOV.value, 0.5f, 0.001f, 179.999f))
				camera.UpdateFOV(m_cameraFOV);

			ImGui::SameLine();
			if (ImGui::Button("Reset##FOV"))
			{
				m_cameraFOV = Constants::DefaultCameraFOV;
				camera.UpdateFOV(m_cameraFOV);
			}

			entt::handle controlledEntity = m_gameInterface.GetControlledEntity();

			bool isCharacterVisible = (controlledEntity) ? !controlledEntity.any_of<Nz::DisabledComponent>() : false;
			if (ImGui::Checkbox("Show character", &isCharacterVisible))
			{
				if (controlledEntity)
				{
					if (!controlledEntity.any_of<Nz::DisabledComponent>())
					{
						// Disable the player entity to make their shadow disappear
						controlledEntity.emplace<Nz::DisabledComponent>();
						// and mask other entities (such as player name) too
						camera.UpdateRenderMask(Constants::RenderMask3D & ~Constants::RenderMaskLocalPlayer);
					}
					else
					{
						controlledEntity.erase<Nz::DisabledComponent>();
						camera.UpdateRenderMask(Constants::RenderMask3D);
					}
				}
			}

			ImGui::Separator();

			ImGui::Checkbox("Cubemap", &m_screenshotCubemap);

			ImGui::DragScalar("Width", ImGuiDataType_U32, &m_screenshotSize.x);
			if (!m_screenshotCubemap)
			{
				ImGui::SameLine();
				ImGui::Text("x");
				ImGui::SameLine();
				ImGui::DragScalar("Height", ImGuiDataType_U32, &m_screenshotSize.y);
			}

			if (ImGui::Button("Screenshot"))
			{
				auto& taskScheduler = m_gameInterface.GetApplication().GetComponent<Nz::TaskSchedulerAppComponent>();
				if (m_screenshotCubemap)
					TakeCubemapScreenshot(taskScheduler, m_gameInterface.GetWorld(), m_cameraEntity, m_screenshotSize.x);
				else
					TakeScreenshot(taskScheduler, m_gameInterface.GetWorld(), m_cameraEntity, m_screenshotSize);
			}
		}
		ImGui::End();
#endif

		cameraNode.SetTransform(cameraPos, cameraRot);
	}

	void CameraTool::TakeCubemapScreenshot(Nz::TaskScheduler& taskScheduler, Nz::EnttWorld& world, entt::handle referenceCamera, Nz::UInt32 size)
	{
		if (!std::filesystem::is_directory("screenshots"))
			std::filesystem::create_directory("screenshots");

		Nz::GpuDevice& gpuDevice = *Nz::Graphics::Instance()->GetGpuDevice();

		Nz::TextureInfo screenshotTextureInfo;
		screenshotTextureInfo.width = size;
		screenshotTextureInfo.height = size;
		screenshotTextureInfo.usageFlags = Nz::TextureUsage::ColorAttachment | Nz::TextureUsage::TransferSource | Nz::TextureUsage::ShaderSampling;
		screenshotTextureInfo.pixelFormat = Nz::PixelFormat::RGBA8;
		screenshotTextureInfo.type = Nz::ImageType::E2D;
		screenshotTextureInfo.layerCount = 1;
		screenshotTextureInfo.levelCount = 1;

		struct FaceData
		{
			entt::handle cameraEntity;
			std::shared_ptr<Nz::Texture> targetTexture;
		};

		Nz::Vector3f cameraPosition = referenceCamera.get<Nz::NodeComponent>().GetGlobalPosition();

		Nz::EnumArray<Direction, FaceData> faces;
		for (auto&& [dir, face] : faces.iter_kv())
		{
			face.targetTexture = gpuDevice.InstantiateTexture(screenshotTextureInfo);

			face.cameraEntity = world.CreateEntity();
			face.cameraEntity.emplace<Nz::NodeComponent>(cameraPosition, Nz::Quaternionf::RotationBetween(Nz::Vector3f::Forward(), s_dirNormals[dir]));

			auto& screenshotCamera = face.cameraEntity.emplace<Nz::CameraComponent>(referenceCamera.get<Nz::CameraComponent>());
			screenshotCamera.UpdateFOV(Nz::DegreeAngle(90.0f));
			screenshotCamera.UpdateTarget(std::make_shared<Nz::RenderTexture>(face.targetTexture));
		}

		world.Update(Nz::Time::Zero());

		std::time_t currentTime = std::time(nullptr);
		std::tm* time = std::localtime(&currentTime);

		spdlog::info("cubemap screenshot taken...");

		for (auto&& [dir, face] : faces.iter_kv())
		{
			std::string filename = fmt::format("screenshots/tsom_{:04}{:02}{:02}_{:02}{:02}{:02}_{}.png", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, s_dirNames[dir]);

			std::unique_ptr<Nz::GpuAsyncCommands> asyncCommands = gpuDevice.InstantiateAsyncCommands(Nz::QueueType::Graphics);
			face.targetTexture->Download(*asyncCommands, Nz::TextureLayout::ColorOutput, [&taskScheduler, filename = std::move(filename)](Nz::Image&& screenshotImage) mutable
			{
				taskScheduler.AddTask([filename = std::move(filename), screenshot = std::move(screenshotImage)]() mutable
				{
					screenshot.SaveToFile(Nz::Utf8Path(filename));
					spdlog::info("saved screenshot to {}", filename);
				});
			});

			asyncCommands->AddCompletionCallback([screenshotTexture = face.targetTexture]{}); //< keep texture alive

			gpuDevice.SubmitAsyncCommands(std::move(asyncCommands));
		}

		for (auto&& [dir, face] : faces.iter_kv())
			face.cameraEntity.destroy();
	}

	void CameraTool::TakeScreenshot(Nz::TaskScheduler& taskScheduler, Nz::EnttWorld& world, entt::handle referenceCamera, const Nz::Vector2ui32& size)
	{
		if (!std::filesystem::is_directory("screenshots"))
			std::filesystem::create_directory("screenshots");

		Nz::GpuDevice& gpuDevice = *Nz::Graphics::Instance()->GetGpuDevice();

		Nz::TextureInfo screenshotTextureInfo;
		screenshotTextureInfo.width = size.x;
		screenshotTextureInfo.height = size.y;
		screenshotTextureInfo.usageFlags = Nz::TextureUsage::ColorAttachment | Nz::TextureUsage::TransferSource | Nz::TextureUsage::ShaderSampling;
		screenshotTextureInfo.pixelFormat = Nz::PixelFormat::RGBA8;
		screenshotTextureInfo.type = Nz::ImageType::E2D;
		screenshotTextureInfo.layerCount = 1;
		screenshotTextureInfo.levelCount = 1;

		std::shared_ptr<Nz::Texture> screenshotTexture = gpuDevice.InstantiateTexture(screenshotTextureInfo);

		entt::handle screenshotCameraEntity = world.CreateEntity();
		screenshotCameraEntity.emplace<Nz::NodeComponent>(referenceCamera.get<Nz::NodeComponent>());
		auto& screenshotCamera = screenshotCameraEntity.emplace<Nz::CameraComponent>(referenceCamera.get<Nz::CameraComponent>());

		screenshotCamera.UpdateTarget(std::make_shared<Nz::RenderTexture>(screenshotTexture));

		world.Update(Nz::Time::Zero());

		screenshotCameraEntity.destroy();

		std::time_t currentTime = std::time(nullptr);
		std::tm* time = std::localtime(&currentTime);

		std::string filename = fmt::format("screenshots/tsom_{:04}{:02}{:02}_{:02}{:02}{:02}.png", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

		spdlog::info("screenshot taken...");

		std::unique_ptr<Nz::GpuAsyncCommands> asyncCommands = gpuDevice.InstantiateAsyncCommands(Nz::QueueType::Graphics);
		screenshotTexture->Download(*asyncCommands, Nz::TextureLayout::ColorOutput, [&taskScheduler, filename = std::move(filename)](Nz::Image&& screenshotImage) mutable
		{
			taskScheduler.AddTask([filename = std::move(filename), screenshot = std::move(screenshotImage)]() mutable
			{
				screenshot.SaveToFile(Nz::Utf8Path(filename));
				spdlog::info("saved screenshot to {}", filename);
			});
		});

		asyncCommands->AddCompletionCallback([screenshotTexture]{}); //< keep texture alive

		gpuDevice.SubmitAsyncCommands(std::move(asyncCommands));
	}
}
