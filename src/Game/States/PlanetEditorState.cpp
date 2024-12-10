// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/PlanetEditorState.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <CommonLib/Planet.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Primitive.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PipelinePassList.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/SimpleLabelWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <charconv>

namespace tsom
{
	PlanetEditorState::PlanetEditorState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_cameraRotation(-45.f, 180.f, 0.f)
	{
		auto& stateData = GetStateData();

		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>(Nz::Vector3f(0.f, 150.f, -75.f), m_cameraRotation);

			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
			cameraComponent.UpdateZNear(0.1f);
			cameraComponent.UpdateZFar(2500.f);
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
					if (m_shouldFreeMouse)
						m_shouldFreeMouse = false;
					else if (m_escapeMenu->IsVisible())
						m_escapeMenu->Hide();
					else
						m_escapeMenu->Show();

					UpdateMouseLock();
					break;
				}

				case Nz::Keyboard::Scancode::F1:
				{
					m_shouldFreeMouse = !m_shouldFreeMouse;
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

		m_planet = std::make_unique<Planet>(*stateData.app, 1.f, 16.f, 9.81f);
		m_planetEntities = std::make_unique<ClientChunkEntities>(*stateData.app, *stateData.world, *m_planet, *stateData.blockLibrary);
		m_planetEntities->SetParentEntity(m_planetParentEntity);
		m_planetEntities->EnableCollisionGeneration(false);

		m_widgetLayout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);
		m_widgetLayout->EnableBackground(true);
		m_widgetLayout->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.2f));

		Nz::SimpleLabelWidget* label = m_widgetLayout->Add<Nz::SimpleLabelWidget>();
		label->SetText("F1 to lock/unlock mouse");

		Nz::BoxLayout* cornerRadiusLayout = m_widgetLayout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);
		{
			Nz::SimpleLabelWidget* cornerRadiusLabel = cornerRadiusLayout->Add<Nz::SimpleLabelWidget>();
			cornerRadiusLabel->SetText("Corner radius:");

			m_cornerRadiusArea = cornerRadiusLayout->Add<Nz::TextAreaWidget>();
			m_cornerRadiusArea->SetCharacterFilter([](char32_t c) { return c >= U'0' && c <= U'9'; });
			m_cornerRadiusArea->EnableBackground(true);
			m_cornerRadiusArea->SetBackgroundColor(Nz::Color::White());
			m_cornerRadiusArea->SetTextColor(Nz::Color::Black());
			m_cornerRadiusArea->SetText("0");
		}

		Nz::BoxLayout* sizeLayout = m_widgetLayout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);
		{
			Nz::SimpleLabelWidget* sizeLabel = sizeLayout->Add<Nz::SimpleLabelWidget>();
			sizeLabel->SetText("Chunk count:");

			Nz::BoxLayout* sizeAreaLayout = sizeLayout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

			m_chunkCountXArea = sizeAreaLayout->Add<Nz::TextAreaWidget>();
			m_chunkCountXArea->SetCharacterFilter([](char32_t c) { return c >= U'0' && c <= U'9'; });
			m_chunkCountXArea->EnableBackground(true);
			m_chunkCountXArea->SetBackgroundColor(Nz::Color::White());
			m_chunkCountXArea->SetTextColor(Nz::Color::Black());
			m_chunkCountXArea->SetText("5");

			Nz::SimpleLabelWidget* sep1Label = sizeAreaLayout->Add<Nz::SimpleLabelWidget>();
			sep1Label->SetText("x");

			m_chunkCountYArea = sizeAreaLayout->Add<Nz::TextAreaWidget>();
			m_chunkCountYArea->SetCharacterFilter([](char32_t c) { return c >= U'0' && c <= U'9'; });
			m_chunkCountYArea->EnableBackground(true);
			m_chunkCountYArea->SetBackgroundColor(Nz::Color::White());
			m_chunkCountYArea->SetTextColor(Nz::Color::Black());
			m_chunkCountYArea->SetText("5");

			Nz::SimpleLabelWidget* sep2Label = sizeAreaLayout->Add<Nz::SimpleLabelWidget>();
			sep2Label->SetText("x");

			m_chunkCountZArea = sizeAreaLayout->Add<Nz::TextAreaWidget>();
			m_chunkCountZArea->SetCharacterFilter([](char32_t c) { return c >= U'0' && c <= U'9'; });
			m_chunkCountZArea->EnableBackground(true);
			m_chunkCountZArea->SetBackgroundColor(Nz::Color::White());
			m_chunkCountZArea->SetTextColor(Nz::Color::Black());
			m_chunkCountZArea->SetText("5");
		}

		Nz::BoxLayout* seedLayout = m_widgetLayout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);
		{
			Nz::SimpleLabelWidget* seedLabel = seedLayout->Add<Nz::SimpleLabelWidget>();
			seedLabel->SetText("Seed:");

			m_seedArea = seedLayout->Add<Nz::TextAreaWidget>();
			m_seedArea->SetCharacterFilter([](char32_t c) { return c >= U'0' && c <= U'9'; });
			m_seedArea->EnableBackground(true);
			m_seedArea->SetBackgroundColor(Nz::Color::White());
			m_seedArea->SetTextColor(Nz::Color::Black());
			m_seedArea->SetText("42");
		}

		Nz::BoxLayout* scriptLayout = m_widgetLayout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);
		{
			Nz::SimpleLabelWidget* scriptLabel = scriptLayout->Add<Nz::SimpleLabelWidget>();
			scriptLabel->SetText("Script:");

			m_scriptArea = scriptLayout->Add<Nz::TextAreaWidget>();
			m_scriptArea->EnableBackground(true);
			m_scriptArea->SetBackgroundColor(Nz::Color::White());
			m_scriptArea->SetTextColor(Nz::Color::Black());
			m_scriptArea->SetText("bob");
		}

		Nz::ButtonWidget* refreshButton = m_widgetLayout->Add<Nz::ButtonWidget>();
		refreshButton->UpdateText(Nz::SimpleTextDrawer::Draw("Refresh", 36));

		refreshButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			RefreshPlanet();
		});

		m_widgetLayout->Resize({ 300.f, 300.f });
		m_widgetLayout->Move({ 10.f, 0.f });
	}

	PlanetEditorState::~PlanetEditorState() = default;

	void PlanetEditorState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		m_shouldFreeMouse = false;

		m_escapeMenu->Hide();

		auto& stateData = GetStateData();
		LayoutWidgets(Nz::Vector2f(stateData.renderTarget->GetSize()));

		auto& config = stateData.app->GetComponent<GameConfigAppComponent>().GetConfig();

		float mouseSensitivity = config.GetFloatValue<float>("Input.MouseSensitivity");
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

		Nz::Mouse::SetRelativeMouseMode(false);
	}

	bool PlanetEditorState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!WidgetState::Update(fsm, elapsedTime))
			return false;

		m_planetEntities->Update();

		auto& stateData = GetStateData();

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

		return true;
	}

	void PlanetEditorState::RefreshPlanet()
	{
		float cornerRadius = 0.f;
		Nz::Vector3ui chunkCount(5, 5, 5);
		Nz::UInt32 seed = 42;

		auto AreaToValue = [](Nz::TextAreaWidget* textArea, auto& value)
		{
			std::string_view str = textArea->GetText();
			std::from_chars(str.data(), str.data() + str.size(), value);
		};

		AreaToValue(m_cornerRadiusArea, cornerRadius);
		AreaToValue(m_chunkCountXArea, chunkCount.x);
		AreaToValue(m_chunkCountYArea, chunkCount.y);
		AreaToValue(m_chunkCountZArea, chunkCount.z);
		AreaToValue(m_seedArea, seed);

		auto& stateData = GetStateData();
		auto& taskScheduler = stateData.app->GetComponent<Nz::TaskSchedulerAppComponent>();

		m_planet->UpdateCornerRadius(cornerRadius);

		m_planet->ClearChunks();
		m_planet->GenerateChunks(*stateData.blockLibrary, taskScheduler, seed, chunkCount, m_scriptArea->GetText());
	}

	void PlanetEditorState::LayoutWidgets(const Nz::Vector2f& /*newSize*/)
	{
		m_escapeMenu->Center();

		m_widgetLayout->CenterVertical();
	}

	void PlanetEditorState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_shouldFreeMouse && !m_escapeMenu->IsVisible();
		Nz::Mouse::SetRelativeMouseMode(m_isMouseLocked);
		if (m_isMouseLocked)
		{
			// FIXME: Expose a way for the canvas to reset keyboard focus
			m_chunkCountXArea->ClearFocus();
			m_chunkCountYArea->ClearFocus();
			m_chunkCountZArea->ClearFocus();
			m_cornerRadiusArea->ClearFocus();
			m_scriptArea->ClearFocus();
			m_seedArea->ClearFocus();
		}
	}
}
