// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/GameAppComponent.hpp>
#include <ClientLib/ClientConfigs.hpp>
#include <ClientLib/ClientFramePipeline.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Rendering/AtmosphereScatteringPipelinePass.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <ClientLib/Systems/CameraFollowerSystem.hpp>
#include <ClientLib/Systems/NetworkMovementInterpolationSystem.hpp>
#include <ClientLib/Systems/PhysicsInterpolationSystem.hpp>
#include <ClientLib/Systems/TransformCopySystem.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/Systems/PlanetSystem.hpp>
#include <CommonLib/Systems/ShipSystem.hpp>
#include <CommonLib/Systems/TickSystem.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/GameConfigs.hpp>
#include <Game/States/BackgroundState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/DebugInfoState.hpp>
#include <Game/States/MenuState.hpp>
#include <Game/States/PlanetEditorState.hpp>
#include <Game/States/VersionCheckState.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EntitySystemAppComponent.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Core/Systems/LifetimeSystem.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderWindow.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Systems/RenderSystem.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Platform/MessageBox.hpp>
#include <Nazara/Platform/WindowingAppComponent.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <charconv>

#ifdef TSOM_DEV_TOOLS
#include <imgui.h>
#endif

namespace tsom
{
	namespace
	{
		enum class MandatoryFeature
		{
			BC1_sRGB,
			BC3_sRGB,
			BC4,
			BC5,
			Depth32F,
			PersistentMapping,
			StorageBuffers,

			Max = StorageBuffers
		};

		constexpr Nz::EnumArray<MandatoryFeature, std::string_view> s_featureNames = {
			"BC1_sRGB pixel format",
			"BC3_sRGB pixel format",
			"BC4 pixel format",
			"BC5 pixel format",
			"Depth32F depth-buffers",
			"persistent mapping",
			"storage buffers"
		};
	}

	GameAppComponent::GameAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
	}

	void GameAppComponent::Start()
	{
		// Check if GPU has minimum required specs
		const Nz::GpuDevice& gpuDevice = *Nz::Graphics::Instance()->GetGpuDevice();
		const Nz::GpuDeviceFeatures& renderDeviceFeatures = gpuDevice.GetEnabledFeatures();

		Nz::EnumArray<MandatoryFeature, bool> featureTests = {
			gpuDevice.IsTextureFormatSupported(Nz::PixelFormat::BC1_RGBA_Unorm, Nz::TextureUsage::ShaderSampling),
			gpuDevice.IsTextureFormatSupported(Nz::PixelFormat::BC3_Unorm, Nz::TextureUsage::ShaderSampling),
			gpuDevice.IsTextureFormatSupported(Nz::PixelFormat::BC4_Unorm, Nz::TextureUsage::ShaderSampling),
			gpuDevice.IsTextureFormatSupported(Nz::PixelFormat::BC5_Unorm, Nz::TextureUsage::ShaderSampling),
			gpuDevice.IsTextureFormatSupported(Nz::PixelFormat::Depth32F, Nz::TextureUsage::DepthStencilAttachment),
			renderDeviceFeatures.persistentMapping,
			renderDeviceFeatures.storageBuffers
		};

		// Test if all mandatory features are supported
		if (std::find(featureTests.begin(), featureTests.end(), false) != featureTests.end())
		{
			std::string missingFeatures;
			for (auto&& [feature, supported] : featureTests.iter_kv())
			{
				if (!supported)
				{
					if (!missingFeatures.empty())
						missingFeatures += ", ";

					missingFeatures += s_featureNames[feature];
				}
			}

			const Nz::GpuDeviceInfo& deviceInfo = gpuDevice.GetDeviceInfo();
			Nz::MessageBox requestBox(Nz::MessageBoxType::Error, "Missing GPU features",
				Nz::Format(
					"Your GPU ({}) doesn't seem to support mandatory features for the game (missing {} support).\n"
					"This is required for the game, try to update your drivers.{}",
					deviceInfo.name,
					missingFeatures,
					(deviceInfo.type == Nz::GpuDeviceType::Integrated) ? "\nThe detected GPU seems to be integrated, try to use a dedicated GPU if possible.": ""
				)
			);

			requestBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

			requestBox.Show();
			GetApp().Quit();
			return;
		}

		if (CheckAssets())
		{
			auto& commandLineParams = GetApp().GetCommandLineParameters();

			auto& window = SetupWindow();
			auto& world = SetupWorld();
			auto& swapchain = SetupSwapchain(world, window);

#ifdef TSOM_DEV_TOOLS
			m_imguiRuntime.emplace(GetApp(), window, swapchain);
#endif

			// TODO: Find a better place
			tsom::AtmosphereScatteringPipelinePass::Register(world);

			auto renderWindow = std::make_shared<Nz::RenderWindow>(swapchain);

			SetupCanvas(world, window);
			SetupCamera(renderWindow, world);

			auto& gameConfig = GetApp().GetComponent<GameConfigAppComponent>();

			std::shared_ptr<tsom::StateData> stateData = std::make_shared<tsom::StateData>();
			stateData->app = &GetApp();
			stateData->blockLibrary = &m_blockLibrary.value();
			stateData->canvas = &m_canvas.value();
			stateData->config = &gameConfig.GetConfig();
			stateData->renderTarget = std::move(renderWindow);
			stateData->window = &window;
			stateData->swapchain = &swapchain;
			stateData->world = &world;

			m_fpsLimit = stateData->config->GetIntegerValue<Nz::Int16>(Config::Graphics_FPSLimit);
			m_fpsLimitUpdateSlot.Connect(stateData->config->GetIntegerUpdateSignal(Config::Graphics_FPSLimit), [this](long long newValue)
			{
				m_fpsLimit = Nz::SafeCaster(newValue);
			});

			std::string_view fpsParam;
			if (commandLineParams.GetParameter("fps-limit", &fpsParam))
			{
				if (std::from_chars(fpsParam.data(), fpsParam.data() + fpsParam.size(), m_fpsLimit).ec == std::errc{})
					stateData->config->SetIntegerValue(Config::Graphics_FPSLimit, m_fpsLimit);
				else
					spdlog::error("failed to parse fps-limit parameter (expected an integer, got \"{}\")", fpsParam);
			}

#ifdef TSOM_DEV_TOOLS
			stateData->imgui = &m_imguiRuntime.value();
#endif

			// Window may be destroyed before application ends, be sure to not get a dangling pointer
			m_onWindowDestruction.Connect(window.GetEventHandler().OnDestruction, [stateData](const Nz::WindowEventHandler*)
			{
				stateData->swapchain = nullptr;
				stateData->window = nullptr;
			});

			if (commandLineParams.HasFlag("planet-editor"))
			{
				m_stateMachine.PushState(std::make_shared<tsom::DebugInfoState>(stateData));
				m_stateMachine.PushState(std::make_shared<tsom::PlanetEditorState>(stateData));
			}
			else
			{
				std::shared_ptr<tsom::ConnectionState> connectionState = std::make_shared<tsom::ConnectionState>(stateData);
				stateData->connectionState = connectionState.get();

				m_stateMachine.PushState(std::make_shared<tsom::DebugInfoState>(stateData));
				m_stateMachine.PushState(std::move(connectionState));
				m_stateMachine.PushState(std::make_shared<tsom::BackgroundState>(stateData));
				m_stateMachine.PushState(std::make_shared<tsom::VersionCheckState>(stateData));
				m_stateMachine.PushState(std::make_shared<tsom::MenuState>(stateData));
			}
		}
	}

	void GameAppComponent::Update(Nz::Time elapsedTime)
	{
#ifdef TSOM_DEV_TOOLS
		if (m_imguiRuntime)
		{
			m_imguiRuntime->BeginFrame(elapsedTime);

			ImGui::SetCurrentContext(m_imguiRuntime->GetContext());
		}
#endif

		if (!m_stateMachine.Update(elapsedTime))
		{
			GetApp().Quit();
			return;
		}

#ifdef TSOM_DEV_TOOLS
		if (m_imguiRuntime)
			m_imguiRuntime->EndFrame();
#endif

		// FPS limiting
		if (m_fpsLimit > 0)
		{
			Nz::Time targetDuration = Nz::Time::TickDuration(m_fpsLimit);
			Nz::Time elapsed = m_frameClock.GetElapsedTime();
			if (elapsed < targetDuration)
			{
				Nz::Time sleepTime = targetDuration - elapsed;
				std::this_thread::sleep_for(sleepTime.AsDuration<std::chrono::milliseconds>());
			}
		}
		m_frameClock.Restart();
	}

	bool GameAppComponent::CheckAssets()
	{
		auto& app = GetApp();
		auto& filesystem = app.GetComponent<Nz::FilesystemAppComponent>();

		m_blockLibrary.emplace(app);
		{
			auto callback = [&](const void* ptr, Nz::UInt64 size)
			{
				m_blockLibrary->LoadFromString(std::string_view(reinterpret_cast<const char*>(ptr), Nz::SafeCast<std::size_t>(size)));
				return true;
			};

			if (!filesystem.GetFileContent("CookedAssets/BlockData.json", callback))
			{
				spdlog::critical("failed to load block data (missing assets)");
				app.Quit();
				return false;
			}

			m_blockLibrary->BuildTexture(*Nz::Graphics::Instance()->GetGpuDevice());
		}

		return true;
	}

	void GameAppComponent::SetupCamera(std::shared_ptr<const Nz::RenderTarget> renderTarget, Nz::EnttWorld& world)
	{
		entt::handle camera2D = world.CreateEntity();
		camera2D.emplace<Nz::NodeComponent>();

		auto& filesystem = GetApp().GetComponent<Nz::FilesystemAppComponent>();
		auto passList = filesystem.Load<Nz::PipelinePassList>("CookedAssets/Passes/2d.passlist");

		auto& cameraComponent = camera2D.emplace<Nz::CameraComponent>(std::move(renderTarget), std::move(passList), Nz::ProjectionType::Orthographic);
		cameraComponent.UpdateClearColor(Nz::Color(0.f, 0.f, 0.f, 0.f));
		cameraComponent.UpdateRenderMask(Constants::RenderMask2D);
		cameraComponent.UpdateRenderOrder(1);
	}

	void GameAppComponent::SetupCanvas(Nz::EnttWorld& world, Nz::Window& window)
	{
		m_canvas.emplace(world.GetRegistry(), window, Constants::RenderMaskUI);
		m_canvas->Resize(Nz::Vector2f(window.GetSize()));
		window.GetEventHandler().OnResized.Connect([&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::SizeEvent& sizeEvent)
		{
			m_canvas->Resize(Nz::Vector2f(sizeEvent.width, sizeEvent.height));
		});
	}

	Nz::WindowSwapchain& GameAppComponent::SetupSwapchain(Nz::EnttWorld& world, Nz::Window& window)
	{
		auto& app = GetApp();

		Nz::SwapchainParameters swapchainParams;

		auto& commandLineParams = app.GetCommandLineParameters();
		if (commandLineParams.HasFlag("no-vsync"))
			swapchainParams.presentMode = { Nz::PresentMode::Mailbox, Nz::PresentMode::Immediate };
		else
			swapchainParams.presentMode = { Nz::PresentMode::RelaxedVerticalSync, Nz::PresentMode::VerticalSync };

		auto& renderSystem = world.GetSystem<Nz::RenderSystem>();
		return renderSystem.CreateSwapchain(window, swapchainParams);
	}

	Nz::Window& GameAppComponent::SetupWindow()
	{
		auto& app = GetApp();

		auto& commandLineParams = app.GetCommandLineParameters();
		auto ParseSize = [&](std::string_view parameterName, unsigned int defaultValue)
		{
			std::string_view param;
			unsigned int size = defaultValue;
			if (commandLineParams.GetParameter(parameterName, &param))
			{
				if (auto err = std::from_chars(param.data(), param.data() + param.size(), size); err.ec != std::errc{} || size == 0)
				{
					spdlog::error("failed to parse {0} commandline parameter ({1}) as a strictly positive number", parameterName, param);
					return defaultValue;
				}
			}

			return size;
		};

		unsigned int windowWidth = ParseSize("width", 1920);
		unsigned int windowHeight = ParseSize("height", 1080);

		auto& windowComponent = app.GetComponent<Nz::WindowingAppComponent>();
		return windowComponent.CreateWindow(Nz::VideoMode(windowWidth, windowHeight), "This Space Of Mine");
	}

	Nz::EnttWorld& GameAppComponent::SetupWorld()
	{
		auto& app = GetApp();

		auto& ecsComponent = app.GetComponent<Nz::EntitySystemAppComponent>();
		auto& world = ecsComponent.AddWorld<Nz::EnttWorld>();

		// World systems
		world.AddSystem<AnimationSystem>();
		world.AddSystem<CameraFollowerSystem>(500.f);
		world.AddSystem<NetworkMovementInterpolationSystem>(Constants::TickDuration);
		world.AddSystem<PlanetSystem>();
		world.AddSystem<ShipSystem>();
		world.AddSystem<TickSystem>();
		world.AddSystem<TransformCopySystem>();
		world.AddSystem<Nz::LifetimeSystem>();
		world.AddSystem<Nz::RenderSystem>([this](Nz::ElementRendererRegistry& elementRegistry) { return std::make_unique<ClientFramePipeline>(elementRegistry, *m_blockLibrary); });

		Nz::Physics3DSystem::Settings physSettings = Physics::BuildSettings();
		physSettings.stepSize = Constants::TickDuration;

		auto& physicsSystem = world.AddSystem<Nz::Physics3DSystem>(std::move(physSettings));
		world.AddSystem<PhysicsInterpolationSystem>(physicsSystem);

		return world;
	}
}
