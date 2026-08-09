// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Platform/MessageBox.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialSettings.hpp>
#include <Nazara/Graphics/PredefinedMaterials.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/GameConstants.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ClientAssetLibraryAppComponent::ClientAssetLibraryAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
	}

	bool ClientAssetLibraryAppComponent::CheckAssets()
	{
		auto& app = GetApp();

		std::filesystem::path assetPath = Nz::Utf8Path("CookedAssets");
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
		filesystem.Mount("CookedAssets", Nz::Utf8Path("CookedAssets"));
		filesystem.Mount("scripts", scriptPath);

		Nz::Graphics* graphics = Nz::Graphics::Instance();
		graphics->GetShaderModuleResolver()->RegisterDirectory(Nz::Utf8Path("CookedAssets/Shaders"), true);

		auto& commandLineParams = GetApp().GetCommandLineParameters();
		if (commandLineParams.HasFlag("dev-assets"))
		{
			filesystem.Mount("CookedAssets/Passes", Nz::Utf8Path("assets/Passes"));
			filesystem.Mount("CookedAssets/Shaders", Nz::Utf8Path("assets/Shaders"));

			graphics->GetShaderModuleResolver()->RegisterDirectory(Nz::Utf8Path("assets/Shaders"), true);
		}

		RegisterPBRMaterial();
		return true;
	}

	void ClientAssetLibraryAppComponent::RegisterPBRMaterial()
	{
		Nz::MaterialSettings settings;
		Nz::PredefinedMaterials::AddBasicSettings(settings);
		Nz::PredefinedMaterials::AddPbrSettings(settings);
		settings.AddTextureProperty("AmbientOcclusionMap", Nz::ImageType::E2D);
		settings.AddTextureProperty("MetallicRoughnessMap", Nz::ImageType::E2D);
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("AmbientOcclusionMap", "HasAmbientOcclusionTexture"));
		settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("MetallicRoughnessMap", "MetallicMap", "HasMetallicRoughnessTexture"));

		auto& renderQueueRegistry = Nz::Graphics::Instance()->GetRenderQueueRegistry();
		std::size_t depthQueue = renderQueueRegistry.GetIndex("DepthOpaque");
		std::size_t forwardOpaqueQueue = renderQueueRegistry.GetIndex("ForwardOpaque");
		std::size_t forwardTransparentQueue = renderQueueRegistry.GetIndex("ForwardTransparent");
		std::size_t shadowQueue = renderQueueRegistry.GetIndex("Shadow");

		Nz::MaterialPass forwardPass;
		forwardPass.renderQueue = forwardOpaqueQueue;
		forwardPass.states.depthBuffer = true;
		forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
		forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.PhysicallyBased"));
		settings.AddPass("ForwardPass", forwardPass);

		Nz::MaterialPass depthPass = forwardPass;
		depthPass.renderQueue = depthQueue;
		depthPass.options[nzsl::Ast::HashOption("DepthPass")] = true;
		settings.AddPass("DepthPass", depthPass);

		Nz::MaterialPass shadowPass = depthPass;
		shadowPass.renderQueue = shadowQueue;
		shadowPass.options[nzsl::Ast::HashOption("ShadowPass")] = true;
		shadowPass.states.depthCompare = Nz::RendererComparison::LessOrEqual; //< TODO: Reverse depth for shadow pass?
		shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
		shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetGpuDevice()->GetEnabledFeatures().depthClamping;
		settings.AddPass("ShadowPass", shadowPass);

		RegisterMaterial("PBRMaterial", std::make_shared<Nz::Material>(std::move(settings), "TSOM.PhysicallyBased"));
	}
}
