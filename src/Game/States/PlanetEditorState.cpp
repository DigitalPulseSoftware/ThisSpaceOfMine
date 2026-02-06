// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/PlanetEditorState.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/GameConfigs.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/PluginManagerAppComponent.hpp>
#include <Nazara/Core/Primitive.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/ImGuiPipelinePass.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PipelinePassList.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Platform/Window.hpp>
#include <Nazara/Renderer/Plugins/ImGuiPlugin.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <numeric>

namespace tsom
{
	PlanetEditorState::PlanetEditorState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_cameraRotation(-45.f, 180.f, 0.f)
	{
		auto& stateData = GetStateData();
		auto& config = stateData.app->GetComponent<GameConfigAppComponent>().GetConfig();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>(Nz::Vector3f(0.f, 20.f, -75.f), m_cameraRotation);

#ifdef TSOM_DEV_TOOLS
			auto passList = filesystem.Load<Nz::PipelinePassList>(stateData.imgui ? "assets/3d_dev.passlist" : "assets/3d.passlist");
#else
			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");
#endif

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.EnableInfiniteZFar(true);
			cameraComponent.EnableReversedZ(true);
			cameraComponent.UpdateClearDepth(0.f);
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
			cameraComponent.UpdateZNear(0.1f);
			cameraComponent.UpdateZFar(10000.f); //< when infinite zfar is enabled, zfar is used as a limit for directional lights
		}

		m_sunLightEntity = CreateEntity();
		{
			m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(tsom::Constants::RenderMask3D);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(4096);
			dirLight.UpdateEnergy(5.f);
		}

		m_skyboxEntity = CreateEntity();
		{
			// Create a new material (custom properties + shaders) for the skybox
			Nz::MaterialSettings skyboxSettings;
			skyboxSettings.AddValueProperty<Nz::Color>("BaseColor", Nz::Color::White());
			skyboxSettings.AddTextureProperty("BaseColorMap", Nz::ImageType::Cubemap);
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("BaseColorMap", "HasBaseColorTexture"));
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("BaseColor"));

			// Setup only a forward pass (using the SkyboxMaterial module)
			Nz::MaterialPass forwardPass;
			forwardPass.states.depthBuffer = true;
			forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
			forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "SkyboxMaterial"));
			skyboxSettings.AddPass("ForwardPass", forwardPass);

			// Finalize the material (using SkyboxMaterial module as a reference for shader reflection)
			std::shared_ptr<Nz::Material> skyboxMaterial = std::make_shared<Nz::Material>(std::move(skyboxSettings), "SkyboxMaterial");

			// Instantiate the material to use it, and configure it (texture + cull front faces as the render is from the inside)
			std::shared_ptr<Nz::MaterialInstance> skyboxMat = skyboxMaterial->Instantiate();
			skyboxMat->SetTextureProperty("BaseColorMap", filesystem.Open<Nz::TextureAsset>("assets/skybox-space.png", { .sRGB = true }, Nz::CubemapParams{}));
			skyboxMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.faceCulling = Nz::FaceCulling::Front;
				return true;
			});

			// Create a cube mesh with only position
			Nz::MeshParams meshPrimitiveParams;
			meshPrimitiveParams.vertexDeclaration = Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ);

			std::shared_ptr<Nz::GraphicalMesh> skyboxMeshGfx = Nz::GraphicalMesh::Build(Nz::Primitive::Box(Nz::Vector3f::Unit() * 10.f, Nz::Vector2ui(0u), Nz::Matrix4f::Identity(), Nz::Rectf(0.f, 0.f, 1.f, 1.f)), meshPrimitiveParams);

			// Setup the model (mesh + material instance)
			std::shared_ptr<Nz::Model> skyboxModel = std::make_shared<Nz::Model>(std::move(skyboxMeshGfx));
			skyboxModel->SetMaterial(0, skyboxMat);

			// Attach the model to the entity
			m_skyboxEntity.emplace<Nz::GraphicsComponent>(std::move(skyboxModel), tsom::Constants::RenderMask3D);

			// Setup entity position and attach it to the camera (position only, camera rotation does not impact skybox)
			auto& skyboxNode = m_skyboxEntity.emplace<Nz::NodeComponent>();
			skyboxNode.SetInheritRotation(false);
			skyboxNode.SetParent(m_cameraEntity);
		}

		m_onUnhandledKeyPressed.Connect(stateData.canvas->OnUnhandledKeyPressed, [this](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			auto& stateData = GetStateData();

			switch (event.scancode)
			{
				case Nz::Keyboard::Scancode::Escape:
				{
					if (m_lockInputs)
						m_lockInputs = false;
					else if (m_escapeMenu->IsVisible())
						m_escapeMenu->Hide();
					else
						m_escapeMenu->Show();

					UpdateMouseLock();
					break;
				}

				case Nz::Keyboard::Scancode::F1:
				{
					m_lockInputs = !m_lockInputs;
					UpdateMouseLock();
					break;
				}

				case Nz::Keyboard::Scancode::F5:
				{
					RefreshPlanet();
					break;
				}

				default:
					break;
			}
		});

		m_escapeMenu = CreateWidget<EscapeMenu>();
		m_escapeMenu->OnWidgetVisibilityUpdated.Connect([&](const Nz::BaseWidget* /*widget*/, bool /*isVisible*/)
		{
			UpdateMouseLock();
		});

		m_escapeMenu->OnDisconnect.Connect([this](EscapeMenu* /*menu*/)
		{
			GetStateData().app->Quit();
		});

		m_escapeMenu->OnQuitApp.Connect([this](EscapeMenu* /*menu*/)
		{
			GetStateData().app->Quit();
		});

		m_planetParentEntity = stateData.world->CreateEntity();
		m_planetParentEntity.emplace<Nz::NodeComponent>();
		m_planetParentEntity.emplace<VisualEntityComponent>().visualEntity = m_planetParentEntity;

		m_planet = std::make_unique<Planet>(*stateData.app, 0.5f, 16.f, 9.81f);
		for (std::size_t layerIndex = 0; layerIndex < m_planetEntities.size(); ++layerIndex)
		{
			if (!stateData.blockLibrary->IsValidLayer(layerIndex))
				continue;

			m_planetEntities[layerIndex] = std::make_unique<ClientChunkEntities>(*stateData.app, *stateData.config, *stateData.world, *m_planet, *stateData.blockLibrary, layerIndex);
			m_planetEntities[layerIndex]->SetParentEntity(m_planetParentEntity);
			m_planetEntities[layerIndex]->EnableCollisionGeneration(false);
		}

		m_atmosphereEntity = CreateEntity();
		{
			m_atmosphereEntity.emplace<Nz::NodeComponent>().SetParent(m_planetParentEntity);
			m_atmosphereEntity.emplace<AtmosphereScattering>();
		}
	}

	PlanetEditorState::~PlanetEditorState()
	{
		auto& stateData = GetStateData();

		// In case previous chunks were still generating
		auto& taskScheduler = stateData.app->GetComponent<Nz::TaskSchedulerAppComponent>();
		taskScheduler.WaitForTasks();
	}

	void PlanetEditorState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		m_lockInputs = false;

		m_escapeMenu->Hide();

		auto& stateData = GetStateData();
		LayoutWidgets(Nz::Vector2f(stateData.renderTarget->GetSize()));

		auto& config = stateData.app->GetComponent<GameConfigAppComponent>().GetConfig();

		float mouseSensitivity = config.GetFloatValue<float>(Config::Input_MouseSensitivity);
		m_mouseMovedSlot.Connect(stateData.canvas->OnUnhandledMouseMoved, [&, mouseSensitivity](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			auto& stateData = GetStateData();

			float pitchMod = -event.deltaY * mouseSensitivity;
			float yawMod = -event.deltaX * mouseSensitivity;

			m_cameraRotation.pitch += pitchMod;
			m_cameraRotation.yaw += yawMod;
			m_cameraEntity.get<Nz::NodeComponent>().SetRotation(m_cameraRotation);
		});

		UpdateMouseLock();
		RefreshPlanet();
	}

	void PlanetEditorState::Leave(Nz::StateMachine& fsm)
	{
		WidgetState::Leave(fsm);

		if (Nz::Window* window = GetStateData().window)
			window->SetRelativeMouseMode(false);
	}

	bool PlanetEditorState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!WidgetState::Update(fsm, elapsedTime))
			return false;

		for (auto& chunkEntitiesPtr : m_planetEntities)
		{
			if (chunkEntitiesPtr)
				chunkEntitiesPtr->Update();
		}

		auto& stateData = GetStateData();

		if (m_isMouseLocked)
		{
			float cameraSpeed = 5.f * elapsedTime.AsSeconds();
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift))
				cameraSpeed *= 10.f;

			auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
			Nz::Vector3f cameraPos = cameraNode.GetPosition();
			Nz::Quaternionf cameraRot = cameraNode.GetRotation();

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W))
				cameraPos += cameraRot * Nz::Vector3f::Forward() * cameraSpeed;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S))
				cameraPos += cameraRot * Nz::Vector3f::Backward() * cameraSpeed;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A))
				cameraPos += cameraRot * Nz::Vector3f::Left() * cameraSpeed;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D))
				cameraPos += cameraRot * Nz::Vector3f::Right() * cameraSpeed;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::Space))
				cameraPos += Nz::Vector3f::Up() * cameraSpeed;

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LControl))
				cameraPos += Nz::Vector3f::Down() * cameraSpeed;

			cameraNode.SetPosition(cameraPos);
		}

		Chunk* chunk = m_planet->GetChunk({0, 0, 0});
		Nz::Vector3f chunkOffset = m_planet->GetChunkOffset({ 0, 0, 0 });

		/*Nz::DebugDrawer* debugDrawer = m_cameraEntity.get<Nz::CameraComponent>().AccessDebugDrawer();

		for (unsigned int z = 0; z < 7; ++z)
		{
			for (unsigned int y = 0; y < 7; ++y)
			{
				for (unsigned int x = 0; x < 7; ++x)
				{
					BlockIndex blockIndex = chunk->GetBlockContent({ x, y, z });
					auto corners = chunk->ComputeBlockCorners({ x, y, z });
					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();

					if (x == 0 && y == 0 && z == 0)
						debugDrawer->DrawSphere(blockCenter, 0.03f, Nz::Color::Yellow());
					else
						debugDrawer->DrawSphere(blockCenter, 0.03f, blockIndex == EmptyBlockIndex ? Nz::Color::Red() : Nz::Color::Blue());
				}
			}
		}*/

#ifdef TSOM_DEV_TOOLS
		if (stateData.imgui)
		{
			ImGui::SetNextWindowPos({ 60, 60 }, ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Planet settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Press F1 to lock/unlock mouse");
				ImGui::Separator();

				ImGui::SliderFloat("Corner radius", &m_planetSettings.cornerRadius, 0.f, 250.f);

				int chunkCount[3] = {
					int(m_planetSettings.chunkCount.x),
					int(m_planetSettings.chunkCount.y),
					int(m_planetSettings.chunkCount.z)
				};
				if (ImGui::InputInt3("Chunk count", chunkCount))
					m_planetSettings.chunkCount = Nz::Vector3ui(std::max(chunkCount[0], 0), std::max(chunkCount[1], 0), std::max(chunkCount[2], 0));

				int seed = m_planetSettings.seed;
				if (ImGui::InputInt("Seed", &seed))
					m_planetSettings.seed = std::max(seed, 0);

				ImGui::InputText("Generator name", &m_planetSettings.scriptName);

				ImGui::Separator();

				if (ImGui::Button("Update planet"))
					RefreshPlanet();
			}
			ImGui::End();

			ImGui::SetNextWindowPos({ 60, 300 }, ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Atmosphere scattering settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				AtmosphereScattering& atmosphereScattering = m_atmosphereEntity.get<AtmosphereScattering>();

				ImGui::Text("Planet and sun parameters");
				ImGui::Text("(drag or ctrl+click to set a value)");

				float sunDir[] = { atmosphereScattering.sunDir.x, atmosphereScattering.sunDir.y, atmosphereScattering.sunDir.z };
				if (ImGui::SliderFloat3("Sun direction", sunDir, -1.f, 1.f))
				{
					atmosphereScattering.sunDir = Nz::Vector3f(sunDir[0], sunDir[1], sunDir[2]);
					atmosphereScattering.sunDir.Normalize();

					m_sunLightEntity.get<Nz::NodeComponent>().SetRotation(Nz::Quaternionf::RotationBetween(Nz::Vector3f::Forward(), -atmosphereScattering.sunDir));
				}

				float sunIntensity[] = { atmosphereScattering.sunIntensity.x, atmosphereScattering.sunIntensity.y, atmosphereScattering.sunIntensity.z };
				if (ImGui::DragFloat3("Sun intensity", sunIntensity))
					atmosphereScattering.sunIntensity = Nz::Vector3f(sunIntensity[0], sunIntensity[1], sunIntensity[2]);

				float planetDimensions[] = { atmosphereScattering.planetDimensions.x, atmosphereScattering.planetDimensions.y, atmosphereScattering.planetDimensions.z };
				if (ImGui::DragFloat3("Planet dimensions", planetDimensions, 1.0f, 0.0f, 200.f))
					atmosphereScattering.planetDimensions = Nz::Vector3f(planetDimensions[0], planetDimensions[1], planetDimensions[2]);

				ImGui::DragFloat("Atmosphere max height", &atmosphereScattering.atmosphereMaxHeight, 1.0f, 0.0f, 1000.f);
				ImGui::DragFloat("Planet corner radius", &atmosphereScattering.planetCornerRadius, 1.0f, 0.0f, 128.f);

				ImGui::Separator();

				ImGui::Text("Scattering coefficients");

				float rayleighBeta[] = { atmosphereScattering.rayleighBeta.x, atmosphereScattering.rayleighBeta.y, atmosphereScattering.rayleighBeta.z };
				if (ImGui::DragFloat3("Rayleigh beta", rayleighBeta, 0.000001f, 0.f, 1.f, "%.6f"))
					atmosphereScattering.rayleighBeta = Nz::Vector3f(rayleighBeta[0], rayleighBeta[1], rayleighBeta[2]);

				float mieBeta[] = { atmosphereScattering.mieBeta.x, atmosphereScattering.mieBeta.y, atmosphereScattering.mieBeta.z };
				if (ImGui::DragFloat3("Mie beta", mieBeta, 0.000001f, 0.f, 1.f, "%.6f"))
					atmosphereScattering.mieBeta = Nz::Vector3f(mieBeta[0], mieBeta[1], mieBeta[2]);

				float ambientBeta[] = { atmosphereScattering.ambientBeta.x, atmosphereScattering.ambientBeta.y, atmosphereScattering.ambientBeta.z };
				if (ImGui::DragFloat3("Ambient beta", ambientBeta, 0.000001f, 0.f, 1.f, "%.6f"))
					atmosphereScattering.ambientBeta = Nz::Vector3f(ambientBeta[0], ambientBeta[1], ambientBeta[2]);

				float absorptionBeta[] = { atmosphereScattering.absorptionBeta.x, atmosphereScattering.absorptionBeta.y, atmosphereScattering.absorptionBeta.z };
				if (ImGui::DragFloat3("Absorption beta", absorptionBeta, 0.000001f, 0.f, 1.f, "%.6f"))
					atmosphereScattering.absorptionBeta = Nz::Vector3f(absorptionBeta[0], absorptionBeta[1], absorptionBeta[2]);

				ImGui::DragFloat("Mie scattering", &atmosphereScattering.mieScattering, 0.001f, 0.f, 1.f);

				ImGui::Separator();

				ImGui::Text("Scattering heights");

				ImGui::DragFloat("Rayleigh height", &atmosphereScattering.rayleighHeight, 0.1f, 0.f, Nz::MaxValue());
				ImGui::DragFloat("Mie height", &atmosphereScattering.mieHeight, 0.1f, 0.f, Nz::MaxValue());
				ImGui::DragFloat("Absorption height", &atmosphereScattering.heightAbsorption, 0.1f, 0.f, Nz::MaxValue());
				ImGui::DragFloat("Absorption falloff", &atmosphereScattering.absorptionFalloff, 0.1f, 0.f, Nz::MaxValue());

				ImGui::Separator();

				int primaryStepCount = atmosphereScattering.primarySteps;
				if (ImGui::SliderInt("Primary steps", &primaryStepCount, 1, 32))
					atmosphereScattering.primarySteps = primaryStepCount;

				int lightStepCount = atmosphereScattering.lightSteps;
				if (ImGui::SliderInt("Light steps", &lightStepCount, 1, 16))
					atmosphereScattering.lightSteps = lightStepCount;

				ImGui::Separator();

				if (ImGui::Button("Reset values"))
					atmosphereScattering = AtmosphereScattering{};
			}
			ImGui::End();

			if (ImGui::Begin("Debug"))
			{
				ImGui::Text("Chunk mesh build time: %lfms", SurfaceNetsChunk::GetMeshBuildTime() / 1'000'000.0);
				ImGui::Text("Chunk collider build time: %lfms", SurfaceNetsChunk::GetColliderBuildTime() / 1'000'000.0);
			}
			ImGui::End();
		}
#endif

		return true;
	}

	void PlanetEditorState::RefreshPlanet()
	{
		auto& stateData = GetStateData();
		auto& taskScheduler = stateData.app->GetComponent<Nz::TaskSchedulerAppComponent>();

		// In case previous chunks were still generating
		taskScheduler.WaitForTasks();

		m_planet->UpdateCornerRadius(m_planetSettings.cornerRadius);

		SurfaceNetsChunk::ResetTime();

		m_planet->ClearChunks();
		m_planet->AddChunks(*stateData.blockLibrary, m_planetSettings.chunkCount);
		m_planet->GenerateChunks(taskScheduler, m_planetSettings.seed, m_planetSettings.chunkCount, m_planetSettings.scriptName);
	}

	void PlanetEditorState::LayoutWidgets(const Nz::Vector2f& /*newSize*/)
	{
		m_escapeMenu->Center();
	}

	void PlanetEditorState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_lockInputs && !m_escapeMenu->IsVisible();

		if (Nz::Window* window = GetStateData().window)
			window->SetRelativeMouseMode(m_isMouseLocked);
	}
}
