// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/GameAppComponent.hpp>
#include <ClientLib/ClientAssetCooker.hpp>
#include <ClientLib/ClientFramePipeline.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Rendering/AtmosphereScatteringPipelinePass.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <ClientLib/Systems/CameraFollowerSystem.hpp>
#include <ClientLib/Systems/NetworkMovementInterpolationSystem.hpp>
#include <ClientLib/Systems/PhysicsInterpolationSystem.hpp>
#include <ClientLib/Systems/TransformCopySystem.hpp>
#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/Systems/PlanetSystem.hpp>
#include <CommonLib/Systems/ShipSystem.hpp>
#include <Game/GameConfigAppComponent.hpp>
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
		const Nz::RenderDevice& renderDevice = *Nz::Graphics::Instance()->GetRenderDevice();
		const Nz::RenderDeviceFeatures& renderDeviceFeatures = renderDevice.GetEnabledFeatures();

		Nz::EnumArray<MandatoryFeature, bool> featureTests = {
			renderDevice.IsTextureFormatSupported(Nz::PixelFormat::BC1_RGBA_Unorm, Nz::TextureUsage::ShaderSampling),
			renderDevice.IsTextureFormatSupported(Nz::PixelFormat::BC3_Unorm, Nz::TextureUsage::ShaderSampling),
			renderDevice.IsTextureFormatSupported(Nz::PixelFormat::BC4_Unorm, Nz::TextureUsage::ShaderSampling),
			renderDevice.IsTextureFormatSupported(Nz::PixelFormat::BC5_Unorm, Nz::TextureUsage::ShaderSampling),
			renderDevice.IsTextureFormatSupported(Nz::PixelFormat::Depth32F, Nz::TextureUsage::DepthStencilAttachment),
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

			const Nz::RenderDeviceInfo& deviceInfo = renderDevice.GetDeviceInfo();
			Nz::MessageBox requestBox(Nz::MessageBoxType::Error, "Missing GPU features",
				Nz::Format(
					"Your GPU ({}) doesn't seem to support mandatory features for the game (missing {} support).\n"
					"This is required for the game, try to update your drivers.{}",
					deviceInfo.name,
					missingFeatures,
					(deviceInfo.type == Nz::RenderDeviceType::Integrated) ? "\nThe detected GPU seems to be integrated, try to use a dedicated GPU if possible.": ""
				)
			);

			requestBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

			requestBox.Show();
			GetApp().Quit();
			return;
		}

		if (CheckAssets())
		{
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

#ifdef TSOM_DEV_TOOLS
			stateData->imgui = &m_imguiRuntime.value();
#endif

			// Window may be destroyed before application ends, be sure to not get a dangling pointer
			m_onWindowDestruction.Connect(window.GetEventHandler().OnDestruction, [stateData](const Nz::WindowEventHandler*)
			{
				stateData->swapchain = nullptr;
				stateData->window = nullptr;
			});

			auto& commandLineParams = GetApp().GetCommandLineParameters();
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
			GetApp().Quit();

#ifdef TSOM_DEV_TOOLS
		if (m_imguiRuntime)
			m_imguiRuntime->EndFrame();
#endif
	}

	bool GameAppComponent::CheckAssets()
	{
		auto& app = GetApp();

		std::filesystem::path assetPath = Nz::Utf8Path("assets");
		if (!std::filesystem::is_directory(assetPath))
		{
			spdlog::error("assets are missing!");

			if (auto* updater = app.TryGetComponent<UpdaterAppComponent>())
			{
				Nz::MessageBox requestBox(Nz::MessageBoxType::Info, "Missing assets folder", "The assets folder was not found.\nDownload assets?");
				requestBox.AddButton(0, Nz::MessageBoxStandardButton::No);
				requestBox.AddButton(1, Nz::MessageBoxStandardButton::Yes);
				if (auto result = requestBox.Show(); !result)
				{
					spdlog::error("failed to open the prompt message box: {0}!", result.GetError());
					app.Quit();
					return false;
				}
				else if (result.GetValue() != 1)
				{
					app.Quit();
					return false;
				}

				updater->FetchLastVersion(false, [updater](Nz::Result<UpdateInfo, std::string>&& result)
				{
					if (!result)
					{
						Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Asset download failed", "Failed to fetch asset info: " + result.GetError());
						errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

						if (auto result = errorBox.Show(); !result)
							spdlog::error("failed to open the error message box: {0}!", result.GetError());

						Nz::ApplicationBase::Instance()->Quit();
						return;
					}

					updater->OnUpdateFailed.Connect([]
					{
						Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Asset download failed", "Failed to download assets");
						errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

						if (auto result = errorBox.Show(); !result)
							spdlog::error("failed to open the error message box: {0}!", result.GetError());

						Nz::ApplicationBase::Instance()->Quit();
					});

					updater->OnDownloadProgress.Connect([lastPrint = Nz::MillisecondClock()](std::size_t activeDownloadCount, Nz::UInt64 downloaded, Nz::UInt64 total) mutable
					{
						if (lastPrint.RestartIfOver(Nz::Time::Second()))
							spdlog::info("downloading {} file(s) ({}/{}) - {}%", activeDownloadCount, ByteToString(downloaded), ByteToString(total), 100 * downloaded / total);
					});

					updater->OnUpdateStarting.Connect([]
					{
						spdlog::info("update is starting...");
					});

					updater->DownloadAndUpdate(result.GetValue(), true, false, true, true);
				});
			}
			else
			{
				Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Missing assets folder", "The assets folder was not found, it should be located next to the executable.");
				errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

				if (auto result = errorBox.Show(); !result)
					spdlog::error("failed to open the error message box: {0}!", result.GetError());

				app.Quit();
			}

			return false;
		}

		std::filesystem::path scriptPath = Nz::Utf8Path("scripts");
		if (!std::filesystem::is_directory(scriptPath))
		{
			spdlog::critical("scripts are missing!");
			app.Quit();
			return false;
		}

		auto& filesystem = app.GetComponent<Nz::FilesystemAppComponent>();
		filesystem.Mount("assets", assetPath);
		filesystem.Mount("CookedAssets", Nz::Utf8Path("cache/CookedAssets"));
		filesystem.Mount("scripts", scriptPath);

		Nz::Graphics* graphics = Nz::Graphics::Instance();
		graphics->GetShaderModuleResolver()->RegisterDirectory(Nz::Utf8Path("assets/shaders"), true);

		m_blockLibrary.emplace(app);
		
		filesystem.GetFileContent("assets/blocks.json", [&](const void* ptr, Nz::UInt64 size)
		{
			m_blockLibrary->LoadFromString(std::string_view(reinterpret_cast<const char*>(ptr), Nz::SafeCast<std::size_t>(size)));
			return true;
		});

		/*ClientAssetCooker assetCooker(app);
		if (auto result = assetCooker.Cook(*m_blockLibrary); !result)
		{
			spdlog::critical("failed to cook assets: {}!", result.GetError());
			app.Quit();
			return false;
		}*/

		m_blockLibrary->BuildTexture(*Nz::Graphics::Instance()->GetRenderDevice());

		return true;
	}

	void GameAppComponent::SetupCamera(std::shared_ptr<const Nz::RenderTarget> renderTarget, Nz::EnttWorld& world)
	{
		entt::handle camera2D = world.CreateEntity();
		camera2D.emplace<Nz::NodeComponent>();

		auto& filesystem = GetApp().GetComponent<Nz::FilesystemAppComponent>();
		auto passList = filesystem.Load<Nz::PipelinePassList>("assets/2d.passlist");

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
