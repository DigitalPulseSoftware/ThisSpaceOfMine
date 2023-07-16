// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/GameState.hpp>
#include <Game/States/StateData.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <Nazara/Core.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/JoltPhysics3D.hpp>
#include <Nazara/Platform/Window.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/Utility.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

//#define FREEFLIGHT
//#define THIRDPERSON

namespace tsom
{
	GameState::GameState(std::shared_ptr<StateData> stateData) :
	m_stateData(std::move(stateData)),
	m_upCorrection(Nz::Quaternionf::Identity()),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Nz::Time::TickDuration(30))
	{
		auto& filesystem = m_stateData->app->GetComponent<Nz::AppFilesystemComponent>();

		m_cameraEntity = m_stateData->world->CreateEntity();
		{
			m_cameraEntity.emplace<Nz::DisabledComponent>();

			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(m_stateData->swapchain);
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			cameraComponent.UpdateRenderMask(0x0000FFFF);
			cameraComponent.UpdateZNear(0.1f);
		}

		m_planet = std::make_unique<ClientPlanet>(40, 2.f, 16.f);
		RebuildPlanet();

		m_planetEntity.emplace<Nz::DisabledComponent>();

		m_skyboxEntity = m_stateData->world->CreateEntity();
		{
			m_skyboxEntity.emplace<Nz::DisabledComponent>();

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
			skyboxMat->SetTextureProperty("BaseColorMap", filesystem.Load<Nz::Texture>("assets/skybox-space.png", Nz::CubemapParams{}));
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
			m_skyboxEntity.emplace<Nz::GraphicsComponent>(std::move(skyboxModel), 0x0000FFFF);

			// Setup entity position and attach it to the camera (position only, camera rotation does not impact skybox)
			auto& skyboxNode = m_skyboxEntity.emplace<Nz::NodeComponent>();
			skyboxNode.SetInheritRotation(false);
			skyboxNode.SetParent(m_cameraEntity);
		}
	}

	GameState::~GameState()
	{
	}

	void GameState::Enter(Nz::StateMachine& /*fsm*/)
	{
#ifdef FREEFLIGHT
		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		cameraNode.SetPosition(Nz::Vector3f::Up() * (m_planet->GetGridDimensions() * m_planet->GetTileSize() * 0.5f + 1.f));
#endif

		m_controlledEntity = m_stateData->sessionHandler->GetControlledEntity();
		m_onControlledEntityChanged.Connect(m_stateData->sessionHandler->OnControlledEntityChanged, [&](entt::handle entity)
		{
			m_controlledEntity = entity;
		});

		m_cameraEntity.erase<Nz::DisabledComponent>();
		m_planetEntity.erase<Nz::DisabledComponent>();
		m_skyboxEntity.erase<Nz::DisabledComponent>();

		Nz::WindowEventHandler& eventHandler = m_stateData->window->GetEventHandler();

		m_cameraRotation = Nz::EulerAnglesf(-30.f, 0.f, 0.f);
		eventHandler.OnMouseMoved.Connect([&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			// Gestion de la caméra free-fly (Rotation)
			float sensitivity = 0.3f; // Sensibilité de la souris

			// On modifie l'angle de la caméra grâce au déplacement relatif sur X de la souris
			m_cameraRotation.yaw = m_cameraRotation.yaw - event.deltaX * sensitivity;
			m_cameraRotation.yaw.Normalize();

			// Idem, mais pour éviter les problèmes de calcul de la matrice de vue, on restreint les angles
			m_cameraRotation.pitch = Nz::Clamp(m_cameraRotation.pitch - event.deltaY * sensitivity, -89.f, 89.f);

			//auto& playerRotNode = playerCamera.get<Nz::NodeComponent>();
			//playerRotNode.SetRotation(camAngles);
		});

		eventHandler.OnKeyPressed.Connect([&](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			switch (event.virtualKey)
			{
				case Nz::Keyboard::VKey::Add:
					m_planet->UpdateCornerRadius(m_planet->GetCornerRadius() + 1.f);
					fmt::print("Corner radius: {}\n", m_planet->GetCornerRadius());

					RebuildPlanet();
					break;

				case Nz::Keyboard::VKey::Subtract:
					m_planet->UpdateCornerRadius(m_planet->GetCornerRadius() - 1.f);
					fmt::print("Corner radius: {}\n", m_planet->GetCornerRadius());

					RebuildPlanet();
					break;

				case Nz::Keyboard::VKey::Divide:
					m_planet = std::make_unique<ClientPlanet>(m_planet->GetGridDimensions() - 1, m_planet->GetTileSize(), m_planet->GetCornerRadius());
					fmt::print("Dimensions size: {}\n", m_planet->GetGridDimensions());

					RebuildPlanet();
					break;

				case Nz::Keyboard::VKey::Multiply:
					m_planet = std::make_unique<ClientPlanet>(m_planet->GetGridDimensions() + 1, m_planet->GetTileSize(), m_planet->GetCornerRadius());
					fmt::print("Dimensions size: {}\n", m_planet->GetGridDimensions());

					RebuildPlanet();
					break;

				default:
					break;
			}
		});

#ifdef FREEFLIGHT
		eventHandler.OnMouseMoved.Connect([&, camAngles = Nz::EulerAnglesf(0.f, 0.f, 0.f)](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event) mutable
		{
			// Gestion de la caméra free-fly (Rotation)
			float sensitivity = 0.3f; // Sensibilité de la souris

			// On modifie l'angle de la caméra grâce au déplacement relatif sur X de la souris
			camAngles.yaw = camAngles.yaw - event.deltaX * sensitivity;
			camAngles.yaw.Normalize();

			// Idem, mais pour éviter les problèmes de calcul de la matrice de vue, on restreint les angles
			camAngles.pitch = Nz::Clamp(camAngles.pitch - event.deltaY * sensitivity, -89.f, 89.f);

			/*auto& playerRotNode = registry.get<Nz::NodeComponent>(playerRotation);
			playerRotNode.SetRotation(camAngles);*/
			auto& playerRotNode = m_cameraEntity.get<Nz::NodeComponent>();
			playerRotNode.SetRotation(camAngles);
		});
#endif
	}

	void GameState::Leave(Nz::StateMachine& /*fsm*/)
	{
		m_cameraEntity.emplace<Nz::DisabledComponent>();
		m_planetEntity.emplace<Nz::DisabledComponent>();
		m_skyboxEntity.emplace<Nz::DisabledComponent>();
	}

	bool GameState::Update(Nz::StateMachine& /*fsm*/, Nz::Time elapsedTime)
	{
		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}

		auto& debugDrawer = m_stateData->world->GetSystem<Nz::RenderSystem>().GetFramePipeline().GetDebugDrawer();
		//debugDrawer.DrawLine(Nz::Vector3f::Zero(), Nz::Vector3f::Forward() * 20.f, Nz::Color::Green());
		//debugDrawer.DrawLine(Nz::Vector3f::Zero(), Nz::Vector3f::Left() * 20.f, Nz::Color::Green());
		//debugDrawer.DrawLine(Nz::Vector3f::Left() * 20.f, Nz::Vector3f::Left() * 20.f + Nz::Vector3f::Forward() * 10.f, Nz::Color::Green());
		//debugDrawer.DrawLine(Nz::Vector3f::Forward() * 20.f, Nz::Vector3f::Left() * 20.f + Nz::Vector3f::Forward() * 10.f, Nz::Color::Green());

		//debugDrawer.DrawBox(m_skybox.get<Nz::GraphicsComponent>().GetAABB(), Nz::Color::Blue());
		//debugDrawer.DrawFrustum(m_camera.get<Nz::CameraComponent>().GetViewerInsta, Nz::Color::Blue());

		float cameraSpeed = (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift)) ? 20.f : 5.f;
		float updateTime = elapsedTime.AsSeconds();

		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
#ifdef FREEFLIGHT
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space))
			cameraNode.Move(Nz::Vector3f::Up() * cameraSpeed * updateTime, Nz::CoordSys::Global);

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Z))
			cameraNode.Move(Nz::Vector3f::Forward() * cameraSpeed * updateTime, Nz::CoordSys::Local);

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::S))
			cameraNode.Move(Nz::Vector3f::Backward() * cameraSpeed * updateTime, Nz::CoordSys::Local);

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Q))
			cameraNode.Move(Nz::Vector3f::Left() * cameraSpeed * updateTime, Nz::CoordSys::Local);

		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::D))
			cameraNode.Move(Nz::Vector3f::Right() * cameraSpeed * updateTime, Nz::CoordSys::Local);
#else
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::F1) && m_stateData->networkSession->IsConnected())
			m_stateData->networkSession->Disconnect();


#ifdef THIRDPERSON
		cameraNode.SetPosition(characterNode.GetPosition() + rotation * (Nz::Vector3f::Up() * 3.f + Nz::Vector3f::Backward() * 2.f));
		cameraNode.SetRotation(rotation * Nz::Quaternionf(m_cameraRotation));
#else
		if (m_controlledEntity)
		{
			Nz::NodeComponent& characterNode = m_controlledEntity.get<Nz::NodeComponent>();

			/*if (m_cameraRotation.yaw != 0.f)
			{
				Nz::Quaternionf newRotation = characterComponent.GetRotation() * Nz::Quaternionf(Nz::EulerAnglesf(0.f, m_cameraRotation.yaw, 0.f));
				newRotation.Normalize();

				characterComponent.SetRotation(newRotation);
				m_cameraRotation.yaw = 0.f;
			}

			Nz::Quaternionf rotation = characterComponent.GetRotation();*/

			cameraNode.SetPosition(characterNode.GetPosition() + characterNode.GetRotation() * (Nz::Vector3f::Up() * 1.6f));
			//cameraNode.SetRotation(characterNode.GetRotation());
			cameraNode.SetRotation(m_upCorrection * Nz::Quaternionf(m_cameraRotation));
		}
#endif

#endif

		// Raycast
		{
			auto& physSystem = m_stateData->world->GetSystem<Nz::JoltPhysics3DSystem>();
			Nz::Vector3f closestHit, closestNormal;
			if (physSystem.RaycastQuery(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 5.f, [&](const Nz::JoltPhysics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
				{
					if (hitInfo.hitEntity != m_planetEntity)
						return std::nullopt;

					closestHit = hitInfo.hitPosition;
					closestNormal = hitInfo.hitNormal;
					return hitInfo.fraction;
				}))
			{
				debugDrawer.DrawLine(closestHit, closestHit + closestNormal * 0.2f, Nz::Color::Cyan());

				auto [x, y, grid] = m_planet->ComputeGridCell(closestHit, closestNormal, debugDrawer);

				if (grid)
				{
					if (Nz::Mouse::IsButtonPressed(Nz::Mouse::Right))
					{
						grid->UpdateCell(x, y, VoxelCell::Empty);
						RebuildPlanet();
					}
					else if (Nz::Mouse::IsButtonPressed(Nz::Mouse::Left))
					{
						auto [x2, y2, grid] = m_planet->ComputeGridCell(closestHit + closestNormal * m_planet->GetTileSize() * 0.5f, closestNormal, debugDrawer);

						grid->UpdateCell(x2, y2, VoxelCell::Dirt);
						RebuildPlanet();
					}
				}


				//fmt::print("{}\n", fmt::streamed(test));

				//debugDrawer.DrawBox(Nz::Boxf(test - Nz::Vector2f(0.1f), Nz::Vector2f(0.2f)), Nz::Color::Green());
			}
		}

		return true;
	}

	void GameState::OnTick(Nz::Time elapsedTime)
	{
		SendInputs();
	}

	void GameState::RebuildPlanet()
	{
		auto& filesystem = m_stateData->app->GetComponent<Nz::AppFilesystemComponent>();

		if (m_planetEntity)
			m_planetEntity.destroy();

		m_planetEntity = m_stateData->world->CreateEntity();
		{
			Nz::TextureSamplerInfo blockSampler;
			blockSampler.anisotropyLevel = 16;
			blockSampler.magFilter = Nz::SamplerFilter::Nearest;
			blockSampler.minFilter = Nz::SamplerFilter::Nearest;
			blockSampler.wrapModeU = Nz::SamplerWrap::Repeat;
			blockSampler.wrapModeV = Nz::SamplerWrap::Repeat;

			std::shared_ptr<Nz::MaterialInstance> planeMat = Nz::Graphics::Instance()->GetDefaultMaterials().basicMaterial->Instantiate();
			planeMat->SetTextureProperty("BaseColorMap", filesystem.Load<Nz::Texture>("assets/tileset.png"), blockSampler);
			planeMat->UpdatePassesStates([&](Nz::RenderStates& states)
			{
				//states.primitiveMode = Nz::PrimitiveMode::LineList;
			});

			std::shared_ptr<Nz::GraphicalMesh> planetMesh;
			{
				Nz::HighPrecisionClock genClock;
				planetMesh = m_planet->BuildGfxMesh();

				fmt::print("planet mesh generated in {}\n", fmt::streamed(genClock.GetElapsedTime()));
			}

			std::shared_ptr<Nz::Model> planetModel = std::make_shared<Nz::Model>(planetMesh);
			planetModel->SetMaterial(0, planeMat);

			auto& planetGfx = m_planetEntity.emplace<Nz::GraphicsComponent>();
			planetGfx.AttachRenderable(planetModel, 0x0000FFFF);
			
			m_planetEntity.emplace<Nz::NodeComponent>();


			Nz::JoltRigidBody3D::StaticSettings settings;
			{
				Nz::HighPrecisionClock colliderClock;
				settings.geom = m_planet->BuildCollider();
				fmt::print("built collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
			}

			auto& physicsSystem = m_stateData->world->GetSystem<Nz::JoltPhysics3DSystem>();

			auto& planetBody = m_planetEntity.emplace<Nz::JoltRigidBody3DComponent>(physicsSystem.CreateRigidBody(settings));

#if 0
			std::shared_ptr<Nz::Model> colliderModel;
			{
				std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::Graphics::Instance()->GetDefaultMaterials().basicMaterial->Instantiate();
				colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
				colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
					{
						states.primitiveMode = Nz::PrimitiveMode::LineList;
						return true;
					});

				std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(settings.geom->GenerateDebugMesh());
				std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

				colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
				for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
					colliderModel->SetMaterial(i, colliderMat);

				planetGfx.AttachRenderable(std::move(colliderModel), 0x0000FFFF);
			}
#endif
		}
	}

	void GameState::SendInputs()
	{
		Packets::UpdatePlayerInputs inputPacket;
		inputPacket.inputs.jump = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space);
		inputPacket.inputs.moveForward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W);
		inputPacket.inputs.moveBackward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S);
		inputPacket.inputs.moveLeft = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A);
		inputPacket.inputs.moveRight = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D);
		inputPacket.inputs.sprint = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift);

		m_controlledEntity = m_stateData->sessionHandler->GetControlledEntity();
		auto& playerNode = m_controlledEntity.get<Nz::NodeComponent>();

		Nz::Vector3f playerUp = playerNode.GetUp();
		if (Nz::Vector3f previousUp = m_upCorrection * Nz::Vector3f::Up(); !previousUp.ApproxEqual(playerUp, 0.001f))
		{
			m_upCorrection = Nz::Quaternionf::RotationBetween(previousUp, playerUp) * m_upCorrection;
			m_upCorrection.Normalize();
		}

		inputPacket.inputs.orientation = m_upCorrection * Nz::Quaternionf(m_cameraRotation);

		m_stateData->networkSession->SendPacket(inputPacket);
	}
}
