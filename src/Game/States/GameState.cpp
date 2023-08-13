// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/GameState.hpp>
#include <Game/States/StateData.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <ClientLib/Chatbox.hpp>
#include <Nazara/Core.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/JoltPhysics3D.hpp>
#include <Nazara/Math/Ray.hpp>
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

		m_planet = std::make_unique<ClientPlanet>(Nz::Vector3ui(160), 2.f, 16.f);

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
		
		m_controlledEntity = m_stateData->sessionHandler->GetControlledEntity();
		m_onControlledEntityChanged.Connect(m_stateData->sessionHandler->OnControlledEntityChanged, [&](entt::handle entity)
		{
			m_controlledEntity = entity;
		});

		m_onChunkCreate.Connect(m_stateData->sessionHandler->OnChunkCreate, [&](const Packets::ChunkCreate& chunkCreate)
		{
			Nz::Vector3ui indices(chunkCreate.chunkLocX, chunkCreate.chunkLocY, chunkCreate.chunkLocZ);

			Chunk& chunk = m_planet->AddChunk(chunkCreate.chunkId, indices, [&](VoxelBlock* blocks)
			{
				for (Nz::UInt8 blockContent : chunkCreate.content)
					*blocks++ = Nz::SafeCast<VoxelBlock>(blockContent);
			});
		});
		
		m_onChunkDestroy.Connect(m_stateData->sessionHandler->OnChunkDestroy, [&](const Packets::ChunkDestroy& chunkDestroy)
		{
			m_planet->RemoveChunk(chunkDestroy.chunkId);
		});

		m_onChunkUpdate.Connect(m_stateData->sessionHandler->OnChunkUpdate, [&](const Packets::ChunkUpdate& chunkUpdate)
		{
			Chunk* chunk = m_planet->GetChunkByNetworkIndex(chunkUpdate.chunkId);
			for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
				chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<VoxelBlock>(blockIndex));
		});
		
		m_chatBox = std::make_unique<Chatbox>(m_stateData->swapchain, m_stateData->canvas);
		m_chatBox->OnChatMessage.Connect([&](const std::string& message)
		{
			Packets::SendChatMessage messagePacket;
			messagePacket.message = message;

			m_stateData->networkSession->SendPacket(messagePacket);
		});
		
		m_onUnhandledKeyPressed.Connect(m_stateData->canvas->OnUnhandledKeyPressed, [this](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			switch (event.virtualKey)
			{
				case Nz::Keyboard::VKey::Escape:
				{
					if (m_chatBox->IsOpen())
						m_chatBox->Close();

					break;
				}

				case Nz::Keyboard::VKey::Return:
				{
					if (m_chatBox->IsOpen())
					{
						m_chatBox->SendMessage();
						m_chatBox->Close();
					}
					else
						m_chatBox->Open();
				
					break;
				}

				default:
					break;
			}
		});

		m_onChatMessage.Connect(m_stateData->sessionHandler->OnChatMessage, [this](const std::string& message, const std::string& senderName)
		{
			if (!senderName.empty())
			{
				m_chatBox->PrintMessage({
					{ Chatbox::ColorItem(Nz::Color::Yellow()) },
					{ Chatbox::TextItem{ senderName } },
					{ Chatbox::TextItem{ ": " }},
					{ Chatbox::ColorItem(Nz::Color::White()) },
					{ Chatbox::TextItem{ message } }
				});
			}
			else
			{
				m_chatBox->PrintMessage({
					{ Chatbox::TextItem{ message } }
				});
			}
		});

		m_onPlayerLeave.Connect(m_stateData->sessionHandler->OnPlayerLeave, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{ Chatbox::TextItem{ playerName } },
				{ Chatbox::TextItem{ " left the server" } }
			});
		});

		m_onPlayerJoined.Connect(m_stateData->sessionHandler->OnPlayerJoined, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{ Chatbox::TextItem{ playerName } },
				{ Chatbox::TextItem{ " joined the server" } }
			});
		});
	}

	GameState::~GameState()
	{
	}

	void GameState::Enter(Nz::StateMachine& /*fsm*/)
	{
		Nz::Mouse::SetRelativeMouseMode(true);

#ifdef FREEFLIGHT
		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		cameraNode.SetPosition(Nz::Vector3f::Up() * (m_planet->GetGridDimensions().z * m_planet->GetTileSize() * 0.5f + 1.f));
#endif

		m_cameraEntity.remove<Nz::DisabledComponent>();
		m_skyboxEntity.remove<Nz::DisabledComponent>();

		m_planetEntities = std::make_unique<ClientPlanetEntities>(*m_stateData->app, *m_stateData->world, *m_planet);

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
					break;

				case Nz::Keyboard::VKey::Subtract:
					m_planet->UpdateCornerRadius(m_planet->GetCornerRadius() - 1.f);
					fmt::print("Corner radius: {}\n", m_planet->GetCornerRadius());
					break;

				/*case Nz::Keyboard::VKey::Divide:
					m_planet = std::make_unique<ClientPlanet>(m_planet->GetGridDimensions() - 1, m_planet->GetTileSize(), m_planet->GetCornerRadius());
					fmt::print("Dimensions size: {}\n", m_planet->GetGridDimensions());

					RebuildPlanet();
					break;

				case Nz::Keyboard::VKey::Multiply:
					m_planet = std::make_unique<ClientPlanet>(m_planet->GetGridDimensions() + 1, m_planet->GetTileSize(), m_planet->GetCornerRadius());
					fmt::print("Dimensions size: {}\n", m_planet->GetGridDimensions());

					RebuildPlanet();
					break;*/

				default:
					break;
			}
		});

		eventHandler.OnMouseButtonReleased.Connect([&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (event.button != Nz::Mouse::Left && event.button != Nz::Mouse::Right)
				return;

			Nz::Vector3f hitPos, hitNormal;
			auto filter = [&](const Nz::JoltPhysics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
			{
				//if (hitInfo.hitEntity != m_planetEntity)
				//	return std::nullopt;

				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				return hitInfo.fraction;
			};

			auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

			auto& physSystem = m_stateData->world->GetSystem<Nz::JoltPhysics3DSystem>();
			if (physSystem.RaycastQuery(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 10.f, filter))
			{
				if (event.button == Nz::Mouse::Left)
				{
					// Mine
					Nz::Vector3f localPos;
					const Chunk* chunk = m_planet->GetChunkByPosition(hitPos - hitNormal * m_planet->GetTileSize() * 0.25f, &localPos);
					if (!chunk)
						return;

					auto coordinates = chunk->ComputeCoordinates(localPos);
					if (!coordinates)
						return;

					Packets::MineBlock mineBlock;
					mineBlock.chunkId = m_planet->GetChunkNetworkIndex(chunk);
					mineBlock.voxelLoc.x = coordinates->x;
					mineBlock.voxelLoc.y = coordinates->y;
					mineBlock.voxelLoc.z = coordinates->z;

					m_stateData->networkSession->SendPacket(mineBlock);
				}
				else
				{
					// Place
					Nz::Vector3f localPos;
					const Chunk* chunk = m_planet->GetChunkByPosition(hitPos + hitNormal * m_planet->GetTileSize() * 0.25f, &localPos);
					if (!chunk)
						return;

					auto coordinates = chunk->ComputeCoordinates(localPos);
					if (!coordinates)
						return;

					Packets::PlaceBlock placeBlock;
					placeBlock.chunkId = m_planet->GetChunkNetworkIndex(chunk);
					placeBlock.voxelLoc.x = coordinates->x;
					placeBlock.voxelLoc.y = coordinates->y;
					placeBlock.voxelLoc.z = coordinates->z;
					placeBlock.newContent = Nz::SafeCast<Nz::UInt8>(VoxelBlock::Dirt);

					m_stateData->networkSession->SendPacket(placeBlock);
				}
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
		Nz::Mouse::SetRelativeMouseMode(false);

		m_chatBox->Close();

		m_planetEntities.reset();

		m_cameraEntity.emplace<Nz::DisabledComponent>();
		m_skyboxEntity.emplace<Nz::DisabledComponent>();
	}

	bool GameState::Update(Nz::StateMachine& /*fsm*/, Nz::Time elapsedTime)
	{
		m_planetEntities->Update();

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

		float cameraSpeed = (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift)) ? 50.f : 10.f;
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
			Nz::Vector3f hitPos, hitNormal;
			if (physSystem.RaycastQuery(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 10.f, [&](const Nz::JoltPhysics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
				{
					//if (hitInfo.hitEntity != m_planetEntity)
					//	return std::nullopt;

					hitPos = hitInfo.hitPosition;
					hitNormal = hitInfo.hitNormal;
					return hitInfo.fraction;
				}))
			{
				debugDrawer.DrawLine(hitPos, hitPos + hitNormal * 0.2f, Nz::Color::Cyan());

				Nz::Vector3f localPos;
				if (const Chunk* chunk = m_planet->GetChunkByPosition(hitPos - hitNormal * m_planet->GetTileSize() * 0.25f, &localPos))
				{
					auto coordinates = chunk->ComputeCoordinates(localPos);
					if (coordinates)
					{
						auto cornerPos = chunk->ComputeVoxelCorners(*coordinates);
						Nz::Vector3f offset = m_planet->GetChunkOffset(chunk->GetIndices());

						constexpr Nz::EnumArray<Direction, std::array<Nz::BoxCorner, 4>> directionToCorners = {
							// Back
							std::array{ Nz::BoxCorner::NearLeftTop, Nz::BoxCorner::NearRightTop, Nz::BoxCorner::NearRightBottom, Nz::BoxCorner::NearLeftBottom },
							// Down
							std::array{ Nz::BoxCorner::NearLeftBottom, Nz::BoxCorner::FarLeftBottom, Nz::BoxCorner::FarRightBottom, Nz::BoxCorner::NearRightBottom },
							// Front
							std::array{ Nz::BoxCorner::FarLeftTop, Nz::BoxCorner::FarRightTop, Nz::BoxCorner::FarRightBottom, Nz::BoxCorner::FarLeftBottom },
							// Left
							std::array{ Nz::BoxCorner::FarLeftTop, Nz::BoxCorner::NearLeftTop, Nz::BoxCorner::NearLeftBottom, Nz::BoxCorner::FarLeftBottom },
							// Right
							std::array{ Nz::BoxCorner::FarRightTop, Nz::BoxCorner::NearRightTop, Nz::BoxCorner::NearRightBottom, Nz::BoxCorner::FarRightBottom },
							// Up
							std::array{ Nz::BoxCorner::NearLeftTop, Nz::BoxCorner::FarLeftTop, Nz::BoxCorner::FarRightTop, Nz::BoxCorner::NearRightTop }
						};

						auto& corners = directionToCorners[DirectionFromNormal(hitNormal)];

						debugDrawer.DrawLine(offset + cornerPos[corners[0]], offset + cornerPos[corners[1]], Nz::Color::Green());
						debugDrawer.DrawLine(offset + cornerPos[corners[1]], offset + cornerPos[corners[2]], Nz::Color::Green());
						debugDrawer.DrawLine(offset + cornerPos[corners[2]], offset + cornerPos[corners[3]], Nz::Color::Green());
						debugDrawer.DrawLine(offset + cornerPos[corners[3]], offset + cornerPos[corners[0]], Nz::Color::Green());
					}
				}
			}
		}

		return true;
	}

	void GameState::OnTick(Nz::Time elapsedTime)
	{
		SendInputs();
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

		if (m_controlledEntity)
		{
			m_controlledEntity = m_stateData->sessionHandler->GetControlledEntity();
			auto& playerNode = m_controlledEntity.get<Nz::NodeComponent>();

			Nz::Vector3f playerUp = playerNode.GetUp();
			if (Nz::Vector3f previousUp = m_upCorrection * Nz::Vector3f::Up(); !previousUp.ApproxEqual(playerUp, 0.001f))
			{
				m_upCorrection = Nz::Quaternionf::RotationBetween(previousUp, playerUp) * m_upCorrection;
				m_upCorrection.Normalize();
			}

			inputPacket.inputs.orientation = m_upCorrection * Nz::Quaternionf(m_cameraRotation);
		}
		else
			inputPacket.inputs.orientation = Nz::Quaternionf::Identity();

		m_stateData->networkSession->SendPacket(inputPacket);
	}
}
