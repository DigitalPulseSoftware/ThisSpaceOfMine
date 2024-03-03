// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/GameAppComponent.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <ClientLib/Systems/MovementInterpolationSystem.hpp>
#include <ClientLib/Systems/PlayerAnimationSystem.hpp>
#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Utils.hpp>
#include <Game/States/BackgroundState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/DebugInfoState.hpp>
#include <Game/States/MenuState.hpp>
#include <Game/States/ShipEditionState.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EntitySystemAppComponent.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/RenderWindow.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Systems/RenderSystem.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Platform/MessageBox.hpp>
#include <Nazara/Platform/WindowingAppComponent.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <charconv>

namespace tsom
{
	GameAppComponent::GameAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
		if (CheckAssets())
		{
			auto& window = SetupWindow();
			auto& world = SetupWorld();

			auto renderTarget = SetupRenderTarget(world, window);

			SetupCanvas(world, window);
			SetupCamera(renderTarget, world);

			std::shared_ptr<tsom::StateData> stateData = std::make_shared<tsom::StateData>();
			stateData->app = &GetApp();
			stateData->blockLibrary = &m_blockLibrary.value();
			stateData->canvas = &m_canvas.value();
			stateData->renderTarget = std::move(renderTarget);
			stateData->window = &window;
			stateData->world = &world;

			std::shared_ptr<tsom::ConnectionState> connectionState = std::make_shared<tsom::ConnectionState>(stateData);
			stateData->connectionState = connectionState.get();

			m_stateMachine.PushState(std::make_shared<tsom::DebugInfoState>(stateData));
			m_stateMachine.PushState(std::move(connectionState));
			m_stateMachine.PushState(std::make_shared<tsom::BackgroundState>(stateData));
			m_stateMachine.PushState(std::make_shared<tsom::MenuState>(stateData));
		}
	}

	void GameAppComponent::Update(Nz::Time elapsedTime)
	{
		if (!m_stateMachine.Update(elapsedTime))
			GetApp().Quit();
	}

	bool GameAppComponent::CheckAssets()
	{
		auto& app = GetApp();

		std::filesystem::path assetPath = Nz::Utf8Path("assets");
		if (!std::filesystem::is_directory(assetPath))
		{
			fmt::print(fg(fmt::color::red), "assets are missing!\n");

			if (auto* updater = app.TryGetComponent<UpdaterAppComponent>())
			{
				Nz::MessageBox requestBox(Nz::MessageBoxType::Info, "Missing assets folder", "The assets folder was not found.\nDownload assets?");
				requestBox.AddButton(0, Nz::MessageBoxStandardButton::No);
				requestBox.AddButton(1, Nz::MessageBoxStandardButton::Yes);
				if (auto result = requestBox.Show(); !result)
				{
					fmt::print(fg(fmt::color::red), "failed to open the prompt message box: {0}!\n", result.GetError());
					app.Quit();
					return false;
				}
				else if (result.GetValue() != 1)
				{
					app.Quit();
					return false;
				}

				updater->FetchLastVersion([updater](Nz::Result<UpdateInfo, std::string>&& result)
				{
					if (!result)
					{
						Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Asset download failed", "Failed to fetch asset info: " + result.GetError());
						errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

						if (auto result = errorBox.Show(); !result)
							fmt::print(fg(fmt::color::red), "failed to open the error message box: {0}!\n", result.GetError());

						Nz::ApplicationBase::Instance()->Quit();
						return;
					}

					updater->OnUpdateFailed.Connect([]
					{
						Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Asset download failed", "Failed to download assets");
						errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

						if (auto result = errorBox.Show(); !result)
							fmt::print(fg(fmt::color::red), "failed to open the error message box: {0}!\n", result.GetError());

						Nz::ApplicationBase::Instance()->Quit();
					});

					updater->OnDownloadProgress.Connect([lastPrint = Nz::MillisecondClock()](std::size_t activeDownloadCount, Nz::UInt64 downloaded, Nz::UInt64 total) mutable
					{
						if (lastPrint.RestartIfOver(Nz::Time::Second()))
							fmt::print("downloading {} file(s) ({}/{}) - {}%\n", activeDownloadCount, ByteToString(downloaded), ByteToString(total), 100 * downloaded / total);
					});

					updater->OnUpdateStarting.Connect([]
					{
						fmt::print("update is starting...\n");
					});

					updater->DownloadAndUpdate(result.GetValue(), true, false);
				});
			}
			else
			{
				Nz::MessageBox errorBox(Nz::MessageBoxType::Error, "Missing assets folder", "The assets folder was not found, it should be located next to the executable.");
				errorBox.AddButton(0, Nz::MessageBoxStandardButton::Close);

				if (auto result = errorBox.Show(); !result)
					fmt::print(fg(fmt::color::red), "failed to open the error message box: {0}!\n", result.GetError());

				app.Quit();
			}

			return false;
		}

		auto& filesystem = app.GetComponent<Nz::FilesystemAppComponent>();
		filesystem.Mount("assets", assetPath);

		Nz::Graphics* graphics = Nz::Graphics::Instance();
		graphics->GetShaderModuleResolver()->RegisterModuleDirectory(Nz::Utf8Path("assets/shaders"), true);

		m_blockLibrary.emplace(app, *graphics->GetRenderDevice());
		m_blockLibrary->BuildTexture();

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
		m_canvas.emplace(world.GetRegistry(), window.GetEventHandler(), window.GetCursorController().CreateHandle(), Constants::RenderMaskUI);
		m_canvas->Resize(Nz::Vector2f(window.GetSize()));
		window.GetEventHandler().OnResized.Connect([&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::SizeEvent& sizeEvent)
		{
			m_canvas->Resize(Nz::Vector2f(sizeEvent.width, sizeEvent.height));
		});
	}

	std::shared_ptr<Nz::RenderTarget> GameAppComponent::SetupRenderTarget(Nz::EnttWorld& world, Nz::Window& window)
	{
		auto& app = GetApp();

		Nz::SwapchainParameters swapchainParams;

		auto& commandLineParams = app.GetCommandLineParameters();
		if (commandLineParams.HasFlag("no-vsync"))
			swapchainParams.presentMode = { Nz::PresentMode::Mailbox, Nz::PresentMode::Immediate };
		else
			swapchainParams.presentMode = { Nz::PresentMode::RelaxedVerticalSync, Nz::PresentMode::VerticalSync };

		auto& renderSystem = world.GetSystem<Nz::RenderSystem>();
		auto& windowSwapchain = renderSystem.CreateSwapchain(window, swapchainParams);

		return std::make_shared<Nz::RenderWindow>(windowSwapchain);
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
					fmt::print(fg(fmt::color::red), "failed to parse {0} commandline parameter ({1}) as a strictly positive number\n", parameterName, param);
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
		world.AddSystem<MovementInterpolationSystem>(Constants::TickDuration);
		auto& renderSystem = world.AddSystem<Nz::RenderSystem>();

		auto& physicsSystem = world.AddSystem<Nz::Physics3DSystem>();
		{
			auto& physWorld = physicsSystem.GetPhysWorld();
			physWorld.SetGravity(Nz::Vector3f::Zero());
			physWorld.SetStepSize(Constants::TickDuration);

			world.AddSystem<PlanetGravitySystem>(physWorld);
		}

		return world;
	}
}
