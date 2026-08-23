// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/PlanetEditorState.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <CommonLib/SurfaceNetsChunk.hpp>
#include <CommonLib/Planets/RoundCubePlanet.hpp>
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
#include <spdlog/spdlog.h>
#include <numeric>

namespace tsom
{
	PlanetEditorState::PlanetEditorState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_cameraRotation(-45.f, 180.f, 0.f),
	m_chunkGenerator(*GetStateData().app)
	{
		auto& stateData = GetStateData();
		auto& config = stateData.app->GetComponent<GameConfigAppComponent>().GetConfig();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		LoadScripts();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>(Nz::Vector3f(0.f, 20.f, -75.f), m_cameraRotation);

#ifdef TSOM_DEV_TOOLS
			auto passList = filesystem.Load<Nz::PipelinePassList>(stateData.imgui ? "CookedAssets/Passes/3d_dev.passlist" : "CookedAssets/Passes/3d.passlist");
#else
			auto passList = filesystem.Load<Nz::PipelinePassList>("CookedAssets/Passes/3d.passlist");
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
			auto& renderQueueRegistry = Nz::Graphics::Instance()->GetRenderQueueRegistry();
			std::size_t forwardOpaqueQueue = renderQueueRegistry.GetIndex("ForwardOpaque");

			// Create a new material (custom properties + shaders) for the skybox
			Nz::MaterialSettings skyboxSettings;
			skyboxSettings.AddValueProperty<Nz::Color>("BaseColor", Nz::Color::White());
			skyboxSettings.AddTextureProperty("BaseColorMap", Nz::ImageType::Cubemap);
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("BaseColorMap", "HasBaseColorTexture"));
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("BaseColor"));

			// Setup only a forward pass (using the SkyboxMaterial module)
			Nz::MaterialPass forwardPass;
			forwardPass.renderQueue = forwardOpaqueQueue;
			forwardPass.states.depthBuffer = true;
			forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
			forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "SkyboxMaterial"));
			skyboxSettings.AddPass("ForwardPass", forwardPass);

			// Finalize the material (using SkyboxMaterial module as a reference for shader reflection)
			std::shared_ptr<Nz::Material> skyboxMaterial = std::make_shared<Nz::Material>(std::move(skyboxSettings), "SkyboxMaterial");

			// Instantiate the material to use it, and configure it (texture + cull front faces as the render is from the inside)
			std::shared_ptr<Nz::MaterialInstance> skyboxMat = skyboxMaterial->Instantiate();
			skyboxMat->SetTextureProperty("BaseColorMap", filesystem.Open<Nz::TextureAsset>("CookedAssets/Textures/Skybox/GameSkybox.dds", { .sRGB = true }));
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

		m_planet = std::make_unique<RoundCubePlanet>(*stateData.app, *stateData.blockLibrary, 0.5f, 0, 16.f, 9.81f);
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
		RefreshScript();
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

#ifdef TSOM_DEV_TOOLS
		if (stateData.imgui)
		{
			ImGui::SetNextWindowPos({ 60, 60 }, ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Planet settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Press F1 to lock/unlock mouse");
				ImGui::Separator();

				int chunkCount[3] = {
					int(m_planetSettings.chunkCount.x),
					int(m_planetSettings.chunkCount.y),
					int(m_planetSettings.chunkCount.z)
				};
				if (ImGui::InputInt3("Chunk count", chunkCount))
					m_planetSettings.chunkCount = Nz::Vector3ui(std::max(chunkCount[0], 0), std::max(chunkCount[1], 0), std::max(chunkCount[2], 0));

				ImGui::InputText("Generator name", &m_planetSettings.scriptName);

				if (ImGui::Button("Update"))
					RefreshScript();

				ImGui::Separator();

				if (m_planetClass)
				{
					for (const auto& property : m_planetClass->GetProperties())
					{
						switch (property.type)
						{
							case EntityPropertyType::Bool:
							{
								bool& value = std::get<EntityPropertySingleValue<EntityPropertyType::Bool>>(m_planetSettings.properties[property.name]);
								ImGui::Checkbox(property.name.data(), &value);
								break;
							}

							case EntityPropertyType::Float:
							{
								float& value = std::get<EntityPropertySingleValue<EntityPropertyType::Float>>(m_planetSettings.properties[property.name]);
								ImGui::InputFloat(property.name.data(), &value);
								break;
							}

							case EntityPropertyType::Integer:
							{
								Nz::Int64& value = std::get<EntityPropertySingleValue<EntityPropertyType::Integer>>(m_planetSettings.properties[property.name]);
								ImGui::InputScalar(property.name.data(), ImGuiDataType_S64, &value);
								break;
							}

							case EntityPropertyType::String:
							{
								std::string& value = std::get<EntityPropertySingleValue<EntityPropertyType::String>>(m_planetSettings.properties[property.name]);
								ImGui::InputText(property.name.data(), &value);
								break;
							}

							default:
								break;
						}
					}
				}

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

				/*float sunIntensity[] = {atmosphereScattering.sunIntensity.x, atmosphereScattering.sunIntensity.y, atmosphereScattering.sunIntensity.z};
				if (ImGui::DragFloat3("Sun intensity", sunIntensity))
					atmosphereScattering.sunIntensity = Nz::Vector3f(sunIntensity[0], sunIntensity[1], sunIntensity[2]);

				float planetDimensions[] = { atmosphereScattering.planetDimensions.x, atmosphereScattering.planetDimensions.y, atmosphereScattering.planetDimensions.z };
				if (ImGui::DragFloat3("Planet dimensions", planetDimensions, 1.0f, 0.0f, 200.f))
					atmosphereScattering.planetDimensions = Nz::Vector3f(planetDimensions[0], planetDimensions[1], planetDimensions[2]);*/

				const char* items[] = {"Round cube", "Torus"};
				int selectedItem = static_cast<int>(atmosphereScattering.shape);
				if (ImGui::Combo("Atmosphere type", &selectedItem, items, IM_ARRAYSIZE(items)))
				{
					atmosphereScattering.shape = static_cast<AtmosphereScatteringShape>(selectedItem);
				}

				ImGui::DragFloat("Atmosphere max height", &atmosphereScattering.atmosphereMaxHeight, 1.0f, 0.0f, 1000.f);

				switch (atmosphereScattering.shape)
				{
					case AtmosphereScatteringShape::RoundCube:
					{
						ImGui::DragFloat3("Planet size", &atmosphereScattering.shapeSettings.x, 1.0f, 0.0f, 1000.f);
						ImGui::DragFloat("Planet corner radius", &atmosphereScattering.shapeSettings.w, 1.0f, 0.0f, 128.f);
						break;
					}

					case AtmosphereScatteringShape::Torus:
					{
						ImGui::DragFloat("Torus radius", &atmosphereScattering.shapeSettings.x, 1.0f, 0.0f, Nz::MaxValue());
						ImGui::DragFloat("Torus thickness", &atmosphereScattering.shapeSettings.y, 1.0f, 0.0f, Nz::MaxValue());
						break;
					}
				}

				ImGui::Separator();

				ImGui::Text("Scattering coefficients");

				ImGui::DragFloat("Scattering strength", &atmosphereScattering.scatteringStrength, 0.001f, 0.f, 10.f);
				ImGui::DragFloat3("Wavelengths", &atmosphereScattering.waveLengths.x, 0.1f, 0.0f, Nz::MaxValue());
				ImGui::DragFloat("Mie scattering", &atmosphereScattering.mieScattering, 0.001f, 0.f, 1.f);
				ImGui::DragFloat("Mie height", &atmosphereScattering.mieHeight, 0.1f, 0.f, Nz::MaxValue());
				ImGui::DragFloat("Absorption falloff", &atmosphereScattering.densityFalloff, 0.1f, 0.f, Nz::MaxValue());

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

		SurfaceNetsChunk::ResetTime();

		m_planet->ClearChunks();
		m_planet->AddChunks(m_planetSettings.chunkCount);
		m_planet->GenerateChunks(taskScheduler, m_planetSettings.chunkCount, m_planetSettings.scriptName, m_planetSettings.properties);
	}

	void PlanetEditorState::RefreshScript()
	{
		if (auto result = m_chunkGenerator.Load(m_planetSettings.scriptName); !result)
		{
			spdlog::error("failed to load {} script: {}", m_planetSettings.scriptName, result.GetError());
			m_planetClass = {};
			return;
		}

		m_planetClass = m_entityRegistry.FindClass(m_chunkGenerator.GetPlanetType());
		if (!m_planetClass)
		{
			spdlog::error("chunk generator references invalid planet type \"{}\"", m_chunkGenerator.GetPlanetType());
			return;
		}

		for (const auto& property : m_planetClass->GetProperties())
		{
			auto it = m_planetSettings.properties.find(property.name);
			if (it == m_planetSettings.properties.end())
				it = m_planetSettings.properties.emplace(property.name, property.defaultValue).first;
		}
	}

	void PlanetEditorState::LayoutWidgets(const Nz::Vector2f& /*newSize*/)
	{
		m_escapeMenu->Center();
	}

	void PlanetEditorState::LoadScripts()
	{
		auto& stateData = GetStateData();
		m_entityRegistry.RegisterClassLibrary<ClientChunkClassLibrary>(*stateData.app, *stateData.config, *stateData.blockLibrary);
		m_entityRegistry.RegisterClassLibrary<ClientEntityClassLibrary>(*stateData.app);
	}

	void PlanetEditorState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_lockInputs && !m_escapeMenu->IsVisible();

		if (Nz::Window* window = GetStateData().window)
			window->SetRelativeMouseMode(m_isMouseLocked);
	}
}
