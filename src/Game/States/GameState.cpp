// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/GameState.hpp>
#include <Game/States/StateData.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/PlanetGravityComponent.hpp>
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
	constexpr std::array<std::string_view, 5> s_selectableBlocks = { "dirt", "grass", "stone", "snow", "debug" };

	GameState::GameState(std::shared_ptr<StateData> stateData) :
	m_stateData(std::move(stateData)),
	m_selectedBlock(0),
	m_upCorrection(Nz::Quaternionf::Identity()),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_escapeMenu(m_stateData->canvas),
	m_isMouseLocked(true)
	{
		auto& filesystem = m_stateData->app->GetComponent<Nz::AppFilesystemComponent>();

		Nz::Vector2f screenSize = Nz::Vector2f(m_stateData->renderTarget->GetSize());

		m_cameraEntity = m_stateData->world->CreateEntity();
		{
			m_cameraEntity.emplace<Nz::DisabledComponent>();

			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();

			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(m_stateData->renderTarget, std::move(passList));
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			cameraComponent.UpdateRenderMask(0x0000FFFF);
			cameraComponent.UpdateZNear(0.1f);
		}

		m_planet = std::make_unique<ClientPlanet>(Nz::Vector3ui(160), 2.f, 16.f, 9.81f);

		m_sunLightEntity = m_stateData->world->CreateEntity();
		{
			m_sunLightEntity.emplace<Nz::DisabledComponent>();
			m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(0x0000FFFF);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(4096);
		}

		constexpr float InventoryTileSize = 96.f;

		const auto& blockColorMap = m_stateData->blockLibrary->GetBaseColorTexture();

		float offset = screenSize.x / 2.f - (s_selectableBlocks.size() * (InventoryTileSize + 5.f)) * 0.5f;
		for (std::string_view blockName : s_selectableBlocks)
		{
			bool active = m_selectedBlock == m_inventorySlots.size();

			auto& slot = m_inventorySlots.emplace_back();

			BlockIndex blockIndex = m_stateData->blockLibrary->GetBlockIndex(blockName);

			std::shared_ptr<Nz::MaterialInstance> slotMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			slotMat->SetTextureProperty("BaseColorMap", m_stateData->blockLibrary->GetPreviewTexture(blockIndex));

			slot.sprite = std::make_shared<Nz::Sprite>(std::move(slotMat));
			slot.sprite->SetColor((active) ? Nz::Color::White() : Nz::Color::Gray());
			slot.sprite->SetSize({ InventoryTileSize, InventoryTileSize });

			slot.entity = m_stateData->world->CreateEntity();
			slot.entity.emplace<Nz::DisabledComponent>();
			slot.entity.emplace<Nz::GraphicsComponent>(slot.sprite, 0xFFFF0000);

			auto& entityNode = slot.entity.emplace<Nz::NodeComponent>();
			entityNode.SetPosition(offset, 5.f);
			offset += (InventoryTileSize + 5.f);
		}

		m_selectedBlockIndex = m_stateData->blockLibrary->GetBlockIndex(s_selectableBlocks[m_selectedBlock]);

		m_mouseWheelMovedSlot.Connect(m_stateData->window->GetEventHandler().OnMouseWheelMoved, [&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::MouseWheelEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			if (event.delta < 0.f)
			{
				m_selectedBlock++;
				if (m_selectedBlock >= s_selectableBlocks.size())
					m_selectedBlock = 0;
			}
			else
			{
				if (m_selectedBlock > 0)
					m_selectedBlock--;
				else
					m_selectedBlock = s_selectableBlocks.size() - 1;
			}

			m_selectedBlockIndex = m_stateData->blockLibrary->GetBlockIndex(s_selectableBlocks[m_selectedBlock]);

			for (std::size_t i = 0; i < m_inventorySlots.size(); ++i)
				m_inventorySlots[i].sprite->SetColor((i == m_selectedBlock) ? Nz::Color::White() : Nz::Color::Gray());
		});

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
			Nz::TextureParams skyboxTexParams;
			skyboxTexParams.loadFormat = Nz::PixelFormat::RGBA8_SRGB;

			std::shared_ptr<Nz::MaterialInstance> skyboxMat = skyboxMaterial->Instantiate();
			skyboxMat->SetTextureProperty("BaseColorMap", filesystem.Load<Nz::Texture>("assets/skybox-space.png", skyboxTexParams, Nz::CubemapParams{}));
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

			Chunk& chunk = m_planet->AddChunk(chunkCreate.chunkId, indices, [&](BlockIndex* blocks)
			{
				for (Nz::UInt8 blockContent : chunkCreate.content)
					*blocks++ = Nz::SafeCast<BlockIndex>(blockContent);
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
				chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<BlockIndex>(blockIndex));
		});
		
		m_chatBox = std::make_unique<Chatbox>(*m_stateData->renderTarget, m_stateData->canvas);
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
					if (m_escapeMenu.IsVisible())
						m_escapeMenu.Hide();
					else if (m_chatBox->IsOpen())
						m_chatBox->Close();
					else
						m_escapeMenu.Show();

					UpdateMouseLock();
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

					UpdateMouseLock();
					break;
				}

				case Nz::Keyboard::VKey::F2:
				{
					auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

					auto primitive = Nz::Primitive::IcoSphere(2.f, 1);

					auto geom = Nz::JoltCollider3D::Build(primitive);

					Nz::JoltRigidBody3D::DynamicSettings dynSettings(geom, 10.f);
					dynSettings.allowSleeping = false;

					entt::handle debugEntity = m_stateData->world->CreateEntity();
					debugEntity.emplace<PlanetGravityComponent>().planet = m_planet.get();
					debugEntity.emplace<Nz::NodeComponent>(cameraNode.GetPosition());
					debugEntity.emplace<Nz::JoltRigidBody3DComponent>(dynSettings);

					std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::PhysicallyBased);

					std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(primitive);
					std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

					auto colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
					for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
						colliderModel->SetMaterial(i, colliderMat);

					auto& gfxComponent = debugEntity.get_or_emplace<Nz::GraphicsComponent>();
					gfxComponent.AttachRenderable(std::move(colliderModel), 0x0000FFFF);
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
					{
						{ Chatbox::ColorItem{ Nz::Color::Yellow() } },
						{ Chatbox::TextItem{ senderName } },
						{ Chatbox::TextItem{ ": " }},
						{ Chatbox::ColorItem{ Nz::Color::White() } },
						{ Chatbox::TextItem{ message } }
					}
				});
			}
			else
			{
				m_chatBox->PrintMessage({
					{
						{ Chatbox::TextItem{ message } }
					}
				});
			}
		});

		m_onPlayerLeave.Connect(m_stateData->sessionHandler->OnPlayerLeave, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::TextItem{ playerName } },
					{ Chatbox::TextItem{ " left the server" } }
				}
			});
		});

		m_onPlayerJoined.Connect(m_stateData->sessionHandler->OnPlayerJoined, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::TextItem{ playerName } },
					{ Chatbox::TextItem{ " joined the server" } }
				}
			});
		});

		m_escapeMenu.OnDisconnect.Connect([this](EscapeMenu* /*menu*/)
		{
			m_stateData->networkSession->Disconnect();
		});

		m_escapeMenu.OnQuitApp.Connect([this](EscapeMenu* /*menu*/)
		{
			m_stateData->app->Quit();
		});
	}

	GameState::~GameState()
	{
	}

	void GameState::Enter(Nz::StateMachine& /*fsm*/)
	{
#ifdef FREEFLIGHT
		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		cameraNode.SetPosition(Nz::Vector3f::Up() * (m_planet->GetGridDimensions().z * m_planet->GetTileSize() * 0.5f + 1.f));
#endif

		m_cameraEntity.remove<Nz::DisabledComponent>();
		m_sunLightEntity.remove<Nz::DisabledComponent>();
		m_skyboxEntity.remove<Nz::DisabledComponent>();

		for (auto& inventorySlot : m_inventorySlots)
			inventorySlot.entity.remove<Nz::DisabledComponent>();

		m_planetEntities = std::make_unique<ClientChunkEntities>(*m_stateData->app, *m_stateData->world, *m_planet, *m_stateData->blockLibrary);

		Nz::WindowEventHandler& eventHandler = m_stateData->window->GetEventHandler();

		m_cameraRotation = Nz::EulerAnglesf(-30.f, 0.f, 0.f);
		eventHandler.OnMouseMoved.Connect([&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			if (!m_isMouseLocked)
				return;

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

		eventHandler.OnMouseButtonReleased.Connect([&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (!m_isMouseLocked)
				return;

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
					placeBlock.newContent = Nz::SafeCast<Nz::UInt8>(m_selectedBlockIndex);

					m_stateData->networkSession->SendPacket(placeBlock);
				}
			}
		});

#ifdef FREEFLIGHT
		eventHandler.OnMouseMoved.Connect([&, camAngles = Nz::EulerAnglesf(0.f, 0.f, 0.f)](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event) mutable
		{
			if (!m_isMouseLocked)
				return;

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

		UpdateMouseLock();
	}

	void GameState::Leave(Nz::StateMachine& /*fsm*/)
	{
		Nz::Mouse::SetRelativeMouseMode(false);

		m_chatBox->Close();

		m_planetEntities.reset();

		m_cameraEntity.emplace<Nz::DisabledComponent>();
		m_sunLightEntity.emplace<Nz::DisabledComponent>();
		m_skyboxEntity.emplace<Nz::DisabledComponent>();

		for (auto& inventorySlot : m_inventorySlots)
			inventorySlot.entity.emplace<Nz::DisabledComponent>();
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
		if (m_isMouseLocked)
		{
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
		}
#else
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::F1) && m_stateData->networkSession->IsConnected())
			m_stateData->networkSession->Disconnect();

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

#ifdef THIRDPERSON
			cameraNode.SetPosition(characterNode.GetPosition() + characterNode.GetRotation() * (Nz::Vector3f::Up() * 3.f + Nz::Vector3f::Backward() * 2.f));
			cameraNode.SetRotation(characterNode.GetRotation() * Nz::EulerAnglesf(-30.f, 0.f, 0.f));
#else
			cameraNode.SetPosition(characterNode.GetPosition() + characterNode.GetRotation() * (Nz::Vector3f::Up() * 1.6f));
			//cameraNode.SetRotation(characterNode.GetRotation());
			cameraNode.SetRotation(m_upCorrection * Nz::Quaternionf(m_cameraRotation));
#endif
		}

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


		m_controlledEntity = m_stateData->sessionHandler->GetControlledEntity();
		if (m_controlledEntity)
		{
			auto& playerNode = m_controlledEntity.get<Nz::NodeComponent>();

			Nz::Vector3f playerUp = playerNode.GetUp();
			if (Nz::Vector3f previousUp = m_upCorrection * Nz::Vector3f::Up(); !previousUp.ApproxEqual(playerUp, 0.001f))
			{
				m_upCorrection = Nz::Quaternionf::RotationBetween(previousUp, playerUp) * m_upCorrection;
				m_upCorrection.Normalize();
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
		if (m_isMouseLocked)
		{
			inputPacket.inputs.jump = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space);
			inputPacket.inputs.moveForward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W);
			inputPacket.inputs.moveBackward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S);
			inputPacket.inputs.moveLeft = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A);
			inputPacket.inputs.moveRight = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D);
			inputPacket.inputs.sprint = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift);
		}

		if (m_controlledEntity)
		{
			inputPacket.inputs.pitch = m_cameraRotation.pitch;
			inputPacket.inputs.yaw = m_cameraRotation.yaw;
		}

		m_stateData->networkSession->SendPacket(inputPacket);
	}

	void GameState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_chatBox->IsTyping() && !m_escapeMenu.IsVisible();
		Nz::Mouse::SetRelativeMouseMode(m_isMouseLocked);
	}
}
