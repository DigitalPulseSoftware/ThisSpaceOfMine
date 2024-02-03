// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/GameState.hpp>
#include <Game/States/StateData.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/PlanetGravityComponent.hpp>
#include <ClientLib/Chatbox.hpp>
#include <ClientLib/RenderConstants.hpp>
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
#define DEBUG_ROTATION 0

namespace tsom
{
	constexpr std::array<std::string_view, 7> s_selectableBlocks = { "dirt", "grass", "stone", "snow", "stone_bricks", "planks", "debug" };

	GameState::GameState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_selectedBlock(0),
	m_upCorrection(Nz::Quaternionf::Identity()),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_escapeMenu(GetStateData().canvas),
	m_isMouseLocked(true),
	m_nextInputIndex(1)
	{
		auto& stateData = GetStateData();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		Nz::Vector2f screenSize = Nz::Vector2f(stateData.renderTarget->GetSize());

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();

			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			#if defined(FREEFLIGHT) || defined(THIRDPERSON)
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D);
			#else
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
			#endif
			cameraComponent.UpdateZNear(0.1f);
		}

		m_planet = std::make_unique<ClientPlanet>(Nz::Vector3ui(180), 1.f, 16.f, 9.81f);

		m_sunLightEntity = CreateEntity();
		{
			m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(tsom::Constants::RenderMask3D);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(4096);
		}

		constexpr float InventoryTileSize = 96.f;

		const auto& blockColorMap = stateData.blockLibrary->GetBaseColorTexture();

		float offset = screenSize.x / 2.f - (s_selectableBlocks.size() * (InventoryTileSize + 5.f)) * 0.5f;
		for (std::string_view blockName : s_selectableBlocks)
		{
			bool active = m_selectedBlock == m_inventorySlots.size();

			auto& slot = m_inventorySlots.emplace_back();

			BlockIndex blockIndex = stateData.blockLibrary->GetBlockIndex(blockName);

			std::shared_ptr<Nz::MaterialInstance> slotMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			slotMat->SetTextureProperty("BaseColorMap", stateData.blockLibrary->GetPreviewTexture(blockIndex));

			slot.sprite = std::make_shared<Nz::Sprite>(std::move(slotMat));
			slot.sprite->SetColor((active) ? Nz::Color::White() : Nz::Color::sRGBToLinear(Nz::Color::Gray()));
			slot.sprite->SetSize({ InventoryTileSize, InventoryTileSize });

			slot.entity = CreateEntity();
			slot.entity.emplace<Nz::GraphicsComponent>(slot.sprite, tsom::Constants::RenderMaskUI);

			auto& entityNode = slot.entity.emplace<Nz::NodeComponent>();
			entityNode.SetPosition(offset, 5.f);
			offset += (InventoryTileSize + 5.f);
		}

		m_selectedBlockIndex = stateData.blockLibrary->GetBlockIndex(s_selectableBlocks[m_selectedBlock]);

		m_mouseWheelMovedSlot.Connect(stateData.window->GetEventHandler().OnMouseWheelMoved, [&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::MouseWheelEvent& event)
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

			m_selectedBlockIndex = stateData.blockLibrary->GetBlockIndex(s_selectableBlocks[m_selectedBlock]);

			for (std::size_t i = 0; i < m_inventorySlots.size(); ++i)
				m_inventorySlots[i].sprite->SetColor((i == m_selectedBlock) ? Nz::Color::White() : Nz::Color::sRGBToLinear(Nz::Color::Gray()));
		});

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
			m_skyboxEntity.emplace<Nz::GraphicsComponent>(std::move(skyboxModel), tsom::Constants::RenderMask3D);

			// Setup entity position and attach it to the camera (position only, camera rotation does not impact skybox)
			auto& skyboxNode = m_skyboxEntity.emplace<Nz::NodeComponent>();
			skyboxNode.SetInheritRotation(false);
			skyboxNode.SetParent(m_cameraEntity);
		}
		
		m_controlledEntity = stateData.sessionHandler->GetControlledEntity();
		m_onControlledEntityChanged.Connect(stateData.sessionHandler->OnControlledEntityChanged, [&](entt::handle entity)
		{
			m_controlledEntity = entity;
		});

		m_onChunkCreate.Connect(stateData.sessionHandler->OnChunkCreate, [&](const Packets::ChunkCreate& chunkCreate)
		{
			Nz::Vector3ui indices(chunkCreate.chunkLocX, chunkCreate.chunkLocY, chunkCreate.chunkLocZ);

			Chunk& chunk = m_planet->AddChunk(chunkCreate.chunkId, indices, [&](BlockIndex* blocks)
			{
				for (Nz::UInt8 blockContent : chunkCreate.content)
					*blocks++ = Nz::SafeCast<BlockIndex>(blockContent);
			});
		});
		
		m_onChunkDestroy.Connect(stateData.sessionHandler->OnChunkDestroy, [&](const Packets::ChunkDestroy& chunkDestroy)
		{
			m_planet->RemoveChunk(chunkDestroy.chunkId);
		});

		m_onChunkUpdate.Connect(stateData.sessionHandler->OnChunkUpdate, [&](const Packets::ChunkUpdate& chunkUpdate)
		{
			Chunk* chunk = m_planet->GetChunkByNetworkIndex(chunkUpdate.chunkId);
			for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
				chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<BlockIndex>(blockIndex));
		});

		m_onControlledEntityStateUpdate.Connect(stateData.sessionHandler->OnControlledEntityStateUpdate, [&](InputIndex inputIndex, const Packets::EntitiesStateUpdate::ControlledCharacter& characterStates)
		{
			m_referenceRotation = characterStates.referenceRotation;

			// Remove processed inputs
			auto it = std::find_if(m_predictedInputRotations.begin(), m_predictedInputRotations.end(), [&](const InputRotation& inputRotation)
			{
				return IsInputMoreRecent(inputRotation.inputIndex, inputIndex);
			});
			m_predictedInputRotations.erase(m_predictedInputRotations.begin(), it);

#if DEBUG_ROTATION
			Nz::EulerAnglesf currentRotation = m_predictedCameraRotation;
#endif

			// Reconciliate
			m_predictedCameraRotation = Nz::EulerAnglesf(characterStates.cameraPitch, characterStates.cameraYaw, 0.f);
			for (const InputRotation& predictedRotation : m_predictedInputRotations)
			{
				m_predictedCameraRotation.pitch = Nz::Clamp(m_predictedCameraRotation.pitch + predictedRotation.inputRotation.pitch, -89.f, 89.f);
				m_predictedCameraRotation.yaw += predictedRotation.inputRotation.yaw;
				m_predictedCameraRotation.Normalize();
			}

			Nz::Quaternionf characterRotation = m_referenceRotation * Nz::Quaternionf(m_predictedCameraRotation.yaw, Nz::Vector3f::Up());
			characterRotation.Normalize();

			auto& characterNode = m_controlledEntity.get<Nz::NodeComponent>();
			characterNode.SetTransform(characterStates.position, characterRotation);

			Nz::Quaternionf cameraRotation = m_referenceRotation * Nz::Quaternionf(m_predictedCameraRotation);
			cameraRotation.Normalize();

			auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
#ifndef FREEFLIGHT
			cameraNode.SetRotation(cameraRotation);
#endif

#if DEBUG_ROTATION
			Nz::EulerAnglesf err = m_predictedCameraRotation - currentRotation;
			float errAcc = std::abs(err.pitch.value) + std::abs(err.yaw.value) + std::abs(err.roll.value);
			if (errAcc > 0.00001f)
			{
				fmt::print("RECONCILIATION ERROR\n");
				m_predictedCameraRotation = Nz::EulerAnglesf(characterStates.cameraPitch, characterStates.cameraYaw, 0.f);
				fmt::print("Starting rotation: {0}\n", fmt::streamed(m_predictedCameraRotation));
				for (const InputRotation& predictedRotation : m_predictedInputRotations)
				{
					m_predictedCameraRotation.pitch = Nz::Clamp(m_predictedCameraRotation.pitch + predictedRotation.inputRotation.pitch, -89.f, 89.f);
					m_predictedCameraRotation.yaw += predictedRotation.inputRotation.yaw;
					m_predictedCameraRotation.Normalize();
					fmt::print("Adding {0} from input {1} which gives {2}\n", fmt::streamed(predictedRotation.inputRotation), predictedRotation.inputIndex, fmt::streamed(m_predictedCameraRotation));
				}
				fmt::print("Giving final rotation {0}\n", fmt::streamed(m_predictedCameraRotation));

				fmt::print("Error: {0}\n------\n", fmt::streamed(err));
			}
#endif
		});
		
		m_chatBox = std::make_unique<Chatbox>(*stateData.renderTarget, stateData.canvas);
		m_chatBox->OnChatMessage.Connect([&](const std::string& message)
		{
			Packets::SendChatMessage messagePacket;
			messagePacket.message = message;

			stateData.networkSession->SendPacket(messagePacket);
		});
		
		m_onUnhandledKeyPressed.Connect(stateData.canvas->OnUnhandledKeyPressed, [this](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			auto& stateData = GetStateData();

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

					auto primitive = Nz::Primitive::IcoSphere(1.f, 2);

					auto geom = Nz::JoltCollider3D::Build(primitive);

					Nz::JoltRigidBody3D::DynamicSettings dynSettings(geom, 10.f);
					dynSettings.allowSleeping = false;

					entt::handle debugEntity = stateData.world->CreateEntity();
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
					gfxComponent.AttachRenderable(std::move(colliderModel), tsom::Constants::RenderMask3D);
					break;
				}

				case Nz::Keyboard::VKey::F3:
				{
					auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
					Nz::Vector3f pos = cameraNode.GetPosition();

					Nz::Vector3f innerPos;
					Chunk* chunk = m_planet->GetChunkByPosition(pos, &innerPos);
					if (chunk)
					{
						std::optional<Nz::Vector3ui> innerCoordinates = chunk->ComputeCoordinates(innerPos);
						if (innerCoordinates)
							fmt::print("Current position = {0}\n", fmt::streamed(chunk->GetIndices() * Planet::ChunkSize + *innerCoordinates));
					}

					break;
				}

				case Nz::Keyboard::VKey::F4:
				{
					auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

					entt::handle debugEntity = stateData.world->CreateEntity();
					debugEntity.emplace<Nz::NodeComponent>(cameraNode.GetPosition(), cameraNode.GetRotation());
					auto& debugLight = debugEntity.emplace<Nz::LightComponent>();

					auto& spotLight = debugLight.AddLight<Nz::SpotLight>();
					spotLight.EnableShadowCasting(true);
					spotLight.UpdateShadowMapSize(1024);
					break;
				}

				default:
					break;
			}
		});

		m_onChatMessage.Connect(stateData.sessionHandler->OnChatMessage, [this](const std::string& message, const std::string& senderName)
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

		m_onPlayerLeave.Connect(stateData.sessionHandler->OnPlayerLeave, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::TextItem{ playerName } },
					{ Chatbox::TextItem{ " left the server" } }
				}
			});
		});

		m_onPlayerJoined.Connect(stateData.sessionHandler->OnPlayerJoined, [this](const std::string& playerName)
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
			GetStateData().networkSession->Disconnect();
		});

		m_escapeMenu.OnQuitApp.Connect([this](EscapeMenu* /*menu*/)
		{
			GetStateData().app->Quit();
		});
	}

	GameState::~GameState()
	{
	}

	void GameState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

#ifdef FREEFLIGHT
		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		cameraNode.SetPosition(Nz::Vector3f::Up() * (m_planet->GetGridDimensions().z * m_planet->GetTileSize() * 0.5f + 1.f));
#endif

		auto& stateData = GetStateData();

		m_planetEntities = std::make_unique<ClientChunkEntities>(*stateData.app, *stateData.world, *m_planet, *stateData.blockLibrary);

		m_remainingCameraRotation = Nz::EulerAnglesf(0.f, 0.f, 0.f);
		m_predictedCameraRotation = m_remainingCameraRotation;
		m_incomingCameraRotation = Nz::EulerAnglesf::Zero();

		Nz::WindowEventHandler& eventHandler = stateData.window->GetEventHandler();

		m_mouseMovedSlot.Connect(eventHandler.OnMouseMoved, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			auto& stateData = GetStateData();

			constexpr float sensitivity = 0.3f; // Mouse sensitivity

			float pitchMod = -event.deltaY * sensitivity;
			float yawMod = -event.deltaX * sensitivity;

			m_incomingCameraRotation.pitch += pitchMod;
			m_incomingCameraRotation.yaw += yawMod;

#ifdef FREEFLIGHT
			static Nz::EulerAnglesf camAngles = Nz::EulerAnglesf::Zero();

			// Gestion de la caméra free-fly (Rotation)
			// 
			// On modifie l'angle de la caméra grâce au déplacement relatif sur X de la souris
			camAngles.yaw = camAngles.yaw + yawMod;
			camAngles.yaw.Normalize();

			// Idem, mais pour éviter les problèmes de calcul de la matrice de vue, on restreint les angles
			camAngles.pitch = Nz::Clamp(camAngles.pitch + pitchMod, -89.f, 89.f);

			/*auto& playerRotNode = registry.get<Nz::NodeComponent>(playerRotation);
			playerRotNode.SetRotation(camAngles);*/
			auto& playerRotNode = m_cameraEntity.get<Nz::NodeComponent>();
			playerRotNode.SetRotation(camAngles);
#endif
		});

		m_mouseButtonReleasedSlot.Connect(eventHandler.OnMouseButtonReleased, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			if (event.button != Nz::Mouse::Left && event.button != Nz::Mouse::Right)
				return;

			auto& stateData = GetStateData();

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

			auto& physSystem = stateData.world->GetSystem<Nz::JoltPhysics3DSystem>();
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

					stateData.networkSession->SendPacket(mineBlock);
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

					stateData.networkSession->SendPacket(placeBlock);
				}
			}
		});

		UpdateMouseLock();
	}

	void GameState::Leave(Nz::StateMachine& fsm)
	{
		WidgetState::Leave(fsm);

		Nz::Mouse::SetRelativeMouseMode(false);

		m_chatBox->Close();
		m_planetEntities.reset();
	}

	bool GameState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!WidgetState::Update(fsm, elapsedTime))
			return false;

		auto& stateData = GetStateData();
		if (!stateData.networkSession)
			return true;

		m_planetEntities->Update();

		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			m_tickAccumulator -= m_tickDuration;
			OnTick(m_tickDuration, m_tickAccumulator < m_tickDuration);
		}

		auto& debugDrawer = stateData.world->GetSystem<Nz::RenderSystem>().GetFramePipeline().GetDebugDrawer();
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
		if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::F1) && stateData.networkSession->IsConnected())
			stateData.networkSession->Disconnect();

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
			cameraNode.SetPosition(characterNode.GetPosition() + characterNode.GetRotation() * (Nz::Vector3f::Up() * 0.6f));
			//cameraNode.SetRotation(characterNode.GetRotation());
			//cameraNode.SetRotation(Nz::Quaternionf::Normalize(characterNode.GetRotation() * Nz::Quaternionf(m_predictedCameraRotation)));
			cameraNode.SetRotation(Nz::Quaternionf::Normalize(characterNode.GetRotation()));

			Nz::Quaternionf cameraRotation = m_referenceRotation * Nz::Quaternionf(m_predictedCameraRotation);
			cameraRotation.Normalize();

			auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
			cameraNode.SetRotation(cameraRotation);
#endif
		}

#endif

		// Raycast
		{
			auto& physSystem = stateData.world->GetSystem<Nz::JoltPhysics3DSystem>();
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


		m_controlledEntity = stateData.sessionHandler->GetControlledEntity();

		return true;
	}

	void GameState::OnTick(Nz::Time elapsedTime, bool lastTick)
	{
		if (lastTick)
			SendInputs();
	}

	void GameState::SendInputs()
	{
		Packets::UpdatePlayerInputs inputPacket;

		inputPacket.inputs.index = m_nextInputIndex++;

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
			m_remainingCameraRotation.pitch += m_incomingCameraRotation.pitch;
			m_remainingCameraRotation.yaw += m_incomingCameraRotation.yaw;

			m_incomingCameraRotation.pitch = Nz::DegreeAnglef::Zero();
			m_incomingCameraRotation.yaw = Nz::DegreeAnglef::Zero();

			if (!m_remainingCameraRotation.pitch.ApproxEqual(Nz::DegreeAnglef::Zero()) || !m_remainingCameraRotation.yaw.ApproxEqual(Nz::DegreeAnglef::Zero()))
			{
				Nz::DegreeAnglef inputPitch = Nz::DegreeAnglef::Clamp(m_remainingCameraRotation.pitch, -Constants::PlayerRotationSpeed, Constants::PlayerRotationSpeed);
				Nz::DegreeAnglef inputYaw = Nz::DegreeAnglef::Clamp(m_remainingCameraRotation.yaw, -Constants::PlayerRotationSpeed, Constants::PlayerRotationSpeed);

				inputPacket.inputs.pitch = inputPitch;
				inputPacket.inputs.yaw = inputYaw;

				m_remainingCameraRotation.pitch -= inputPitch;
				m_remainingCameraRotation.yaw -= inputYaw;

				m_predictedCameraRotation.pitch = Nz::Clamp(m_predictedCameraRotation.pitch + inputPitch, -89.f, 89.f);
				m_predictedCameraRotation.yaw += inputYaw;
				m_predictedCameraRotation.Normalize();

				m_predictedInputRotations.push_back({
					.inputIndex = inputPacket.inputs.index,
					.inputRotation = Nz::EulerAnglesf(inputPacket.inputs.pitch, inputPacket.inputs.yaw, Nz::DegreeAnglef::Zero())
				});
			}
		}

		GetStateData().networkSession->SendPacket(inputPacket);
	}

	void GameState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_chatBox->IsTyping() && !m_escapeMenu.IsVisible();
		Nz::Mouse::SetRelativeMouseMode(m_isMouseLocked);
	}
}
