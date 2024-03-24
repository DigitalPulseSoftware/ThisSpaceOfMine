// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/GameState.hpp>
#include <ClientLib/BlockSelectionBar.hpp>
#include <ClientLib/Chatbox.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/PrimitiveList.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/FramePipeline.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PipelinePassList.hpp>
#include <Nazara/Graphics/PointLight.hpp>
#include <Nazara/Graphics/SpotLight.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Graphics/Systems/RenderSystem.hpp>
#include <Nazara/Math/Ray.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Platform/Window.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#define DEBUG_ROTATION 0

namespace tsom
{
	GameState::GameState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_upCorrection(Nz::Quaternionf::Identity()),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_nextInputIndex(1),
	m_isCameraThirdPerson(false),
	m_isMouseLocked(true)
	{
		auto& stateData = GetStateData();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();

			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
			cameraComponent.UpdateZNear(0.1f);
		}

		m_planet = std::make_unique<ClientPlanet>(1.f, 16.f, 9.81f);

		m_sunLightEntity = CreateEntity();
		{
			m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(tsom::Constants::RenderMask3D);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(4096);
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
			ChunkIndices indices(chunkCreate.chunkLocX, chunkCreate.chunkLocY, chunkCreate.chunkLocZ);

			m_planet->AddChunk(chunkCreate.chunkId, indices);
		});

		m_onChunkDestroy.Connect(stateData.sessionHandler->OnChunkDestroy, [&](const Packets::ChunkDestroy& chunkDestroy)
		{
			m_planet->RemoveChunk(chunkDestroy.chunkId);
		});

		m_onChunkReset.Connect(stateData.sessionHandler->OnChunkReset, [&](const Packets::ChunkReset& chunkReset)
		{
			Chunk* chunk = m_planet->GetChunkByNetworkIndex(chunkReset.chunkId);
			if (!chunk)
			{
				fmt::print(fg(fmt::color::red), "ChunkReset handler: unknown chunk {}\n", chunkReset.chunkId);
				return;
			}

			chunk->LockWrite();
			chunk->Reset([&](BlockIndex* blocks)
			{
				for (BlockIndex blockContent : chunkReset.content)
					*blocks++ = blockContent;
			});
			chunk->UnlockWrite();
		});

		m_onChunkUpdate.Connect(stateData.sessionHandler->OnChunkUpdate, [&](const Packets::ChunkUpdate& chunkUpdate)
		{
			Chunk* chunk = m_planet->GetChunkByNetworkIndex(chunkUpdate.chunkId);
			chunk->LockWrite();

			for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
				chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<BlockIndex>(blockIndex));

			chunk->UnlockWrite();
		});

		m_onControlledEntityStateUpdate.Connect(stateData.sessionHandler->OnControlledEntityStateUpdate, [&](InputIndex inputIndex, const Packets::EntitiesStateUpdate::ControlledCharacter& characterStates)
		{
			if (!m_controlledEntity)
				return;

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
			cameraNode.SetRotation(cameraRotation);

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

		m_blockSelectionBar = CreateWidget<BlockSelectionBar>(*stateData.blockLibrary);
		m_mouseWheelMovedSlot.Connect(stateData.window->GetEventHandler().OnMouseWheelMoved, [&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::MouseWheelEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			if (event.delta < 0.f)
				m_blockSelectionBar->SelectNext();
			else
				m_blockSelectionBar->SelectPrevious();
		});

		m_escapeMenu = CreateWidget<EscapeMenu>();
		m_escapeMenu->OnWidgetVisibilityUpdated.Connect([&](const Nz::BaseWidget* /*widget*/, bool /*isVisible*/)
		{
			UpdateMouseLock();
		});

		m_chatBox = CreateWidget<Chatbox>();
		m_chatBox->OnChatMessage.Connect([&](const std::string& message)
		{
			Packets::SendChatMessage messagePacket;
			messagePacket.message = message;

			stateData.networkSession->SendPacket(messagePacket);
		});

		LayoutWidgets(Nz::Vector2f(stateData.renderTarget->GetSize()));

		m_onUnhandledKeyPressed.Connect(stateData.canvas->OnUnhandledKeyPressed, [this](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			auto& stateData = GetStateData();

			switch (event.virtualKey)
			{
				case Nz::Keyboard::VKey::Escape:
				{
					if (m_escapeMenu->IsVisible())
						m_escapeMenu->Hide();
					else if (m_chatBox->IsOpen())
						m_chatBox->Close();
					else
						m_escapeMenu->Show();

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

					auto geom = Nz::Collider3D::Build(primitive);

					Nz::RigidBody3D::DynamicSettings dynSettings(geom, 10.f);
					dynSettings.allowSleeping = false;

					entt::handle debugEntity = stateData.world->CreateEntity();
					debugEntity.emplace<PlanetComponent>().planet = m_planet.get();
					debugEntity.emplace<Nz::NodeComponent>(cameraNode.GetPosition());
					debugEntity.emplace<Nz::RigidBody3DComponent>(dynSettings);

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
					if (!m_debugOverlay)
					{
						m_debugOverlay = std::make_shared<DebugOverlay>();
						m_debugOverlay->label = CreateWidget<Nz::LabelWidget>();
						m_debugOverlay->textDrawer.SetCharacterSize(18);
					}
					else
					{
						m_debugOverlay->mode++;
						if (m_debugOverlay->mode > 3)
						{
							// Disable debug overlay
							DestroyWidget(m_debugOverlay->label);
							m_debugOverlay = nullptr;
							break;
						}
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

				case Nz::Keyboard::VKey::F5:
				{
					m_isCameraThirdPerson = !m_isCameraThirdPerson;

					auto& cameraComponent = m_cameraEntity.get<Nz::CameraComponent>();
					if (m_isCameraThirdPerson)
						cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D);
					else
						cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
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

				fmt::print("{0}: {1}\n", senderName, message);
			}
			else
			{
				m_chatBox->PrintMessage({
					{
						{ Chatbox::TextItem{ message } }
					}
				});

				fmt::print("{0}\n", message);
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

			fmt::print("{0} left the server\n", playerName);
		});

		m_onPlayerJoined.Connect(stateData.sessionHandler->OnPlayerJoined, [this](const std::string& playerName)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::TextItem{ playerName } },
					{ Chatbox::TextItem{ " joined the server" } }
				}
			});

			fmt::print("{0} joined the server\n", playerName);
		});

		m_escapeMenu->OnDisconnect.Connect([this](EscapeMenu* /*menu*/)
		{
			GetStateData().networkSession->Disconnect();
		});

		m_escapeMenu->OnQuitApp.Connect([this](EscapeMenu* /*menu*/)
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

		m_chatBox->Close();
		m_escapeMenu->Hide();

		auto& stateData = GetStateData();

		m_planetEntities = std::make_unique<ClientChunkEntities>(*stateData.app, *stateData.world, *m_planet, *stateData.blockLibrary);

		m_remainingCameraRotation = Nz::EulerAnglesf(0.f, 0.f, 0.f);
		m_predictedCameraRotation = m_remainingCameraRotation;
		m_incomingCameraRotation = Nz::EulerAnglesf::Zero();

		m_mouseMovedSlot.Connect(stateData.canvas->OnUnhandledMouseMoved, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			auto& stateData = GetStateData();

			constexpr float sensitivity = 0.3f; // Mouse sensitivity

			float pitchMod = -event.deltaY * sensitivity;
			float yawMod = -event.deltaX * sensitivity;

			m_incomingCameraRotation.pitch += pitchMod;
			m_incomingCameraRotation.yaw += yawMod;
		});

		m_mouseButtonReleasedSlot.Connect(stateData.canvas->OnUnhandledMouseButtonReleased, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			if (event.button != Nz::Mouse::Left && event.button != Nz::Mouse::Right)
				return;

			auto& stateData = GetStateData();

			Nz::Vector3f hitPos, hitNormal;
			auto filter = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
			{
				//if (hitInfo.hitEntity != m_planetEntity)
				//  return std::nullopt;

				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				return hitInfo.fraction;
			};

			auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

			auto& physSystem = stateData.world->GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQuery(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 10.f, filter))
			{
				if (event.button == Nz::Mouse::Left)
				{
					// Mine
					Nz::Vector3f blockPos = hitPos - hitNormal * m_planet->GetTileSize() * 0.25f;
					const Chunk* chunk = m_planet->GetChunk(m_planet->GetChunkIndicesByPosition(blockPos));
					if (!chunk)
						return;

					auto coordinates = chunk->ComputeCoordinates(blockPos);
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
					Nz::Vector3f blockPos = hitPos + hitNormal * m_planet->GetTileSize() * 0.25f;
					const Chunk* chunk = m_planet->GetChunk(m_planet->GetChunkIndicesByPosition(blockPos));
					if (!chunk)
						return;

					auto coordinates = chunk->ComputeCoordinates(blockPos);
					if (!coordinates)
						return;

					Packets::PlaceBlock placeBlock;
					placeBlock.chunkId = m_planet->GetChunkNetworkIndex(chunk);
					placeBlock.voxelLoc.x = coordinates->x;
					placeBlock.voxelLoc.y = coordinates->y;
					placeBlock.voxelLoc.z = coordinates->z;
					placeBlock.newContent = Nz::SafeCast<Nz::UInt8>(m_blockSelectionBar->GetSelectedBlock());

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

		if (m_debugOverlay)
			m_debugOverlay->textDrawer.Clear();

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
		if (m_controlledEntity)
		{
			Nz::NodeComponent& characterNode = m_controlledEntity.get<Nz::NodeComponent>();

			Nz::Vector3f characterPos = characterNode.GetPosition();
			Nz::Quaternionf characterRot = characterNode.GetRotation();

			if (m_isCameraThirdPerson)
			{
				Nz::Quaternionf cameraRot = characterRot * Nz::EulerAnglesf(m_predictedCameraRotation.pitch, 0.f, 0.f);
				cameraRot.Normalize();

				cameraNode.SetPosition(characterPos + characterRot * (Nz::Vector3f::Up() * 1.f) + cameraRot * (Nz::Vector3f::Backward() * 1.f));
				cameraNode.SetRotation(cameraRot);
			}
			else
			{
				cameraNode.SetPosition(characterPos + characterRot * (Nz::Vector3f::Up() * Constants::PlayerCameraHeight));

				Nz::Quaternionf cameraRotation = m_referenceRotation * Nz::Quaternionf(m_predictedCameraRotation);
				cameraRotation.Normalize();

				cameraNode.SetRotation(cameraRotation);
			}

			if (m_debugOverlay)
			{
				m_debugOverlay->textDrawer.AppendText(fmt::format("{0:-^{1}}\n", "Player position", 20));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Position: {0:.3f};{1:.3f};{2:.3f}\n", characterPos.x, characterPos.y, characterPos.z));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Rotation: {0:.3f};{1:.3f};{2:.3f};{3:.3f}\n", characterRot.x, characterRot.y, characterRot.z, characterRot.w));

				Nz::Vector3f up = m_planet->ComputeUpDirection(characterPos);
				float gravity = m_planet->GetGravityFactor(characterPos);

				m_debugOverlay->textDrawer.AppendText(fmt::format("Up direction: {0:.3f};{1:.3f};{2:.3f} - gravity: {3:.2f}\n", up.x, up.y, up.z, gravity));

				ChunkIndices chunkIndices = m_planet->GetChunkIndicesByPosition(characterPos);
				const Chunk* chunk = m_planet->GetChunk(chunkIndices);

				m_debugOverlay->textDrawer.AppendText(fmt::format("Chunk: {0};{1};{2}{3}\n", chunkIndices.x, chunkIndices.y, chunkIndices.z, chunk ? "" : " (not loaded)"));

				if (const Chunk* chunk = m_planet->GetChunk(chunkIndices))
				{
					if (auto coordinates = chunk->ComputeCoordinates(characterPos))
						m_debugOverlay->textDrawer.AppendText(fmt::format("Chunk block: {0};{1};{2}\n", coordinates->x, coordinates->y, coordinates->z));
				}
			}
		}

		// Network info
		if (m_debugOverlay && m_debugOverlay->mode >= 2)
		{
			const auto* connectionInfo = stateData.connectionState->GetConnectionInfo();
			if (connectionInfo)
			{
				Nz::UInt32 downloadSpeed = Nz::UInt32(std::max(std::round(connectionInfo->downloadSpeed.GetAverageValue()), 0.0));
				Nz::UInt32 uploadSpeed = Nz::UInt32(std::max(std::round(connectionInfo->uploadSpeed.GetAverageValue()), 0.0));

				m_debugOverlay->textDrawer.AppendText(fmt::format("{0:-^{1}}\n", "Network", 20));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Ping: {0}ms\n", connectionInfo->peerInfo.ping));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Download speed: {0}\n", ByteToString(downloadSpeed, true)));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Upload speed: {0}\n", ByteToString(uploadSpeed, true)));
				if (m_debugOverlay->mode >= 3)
				{
					m_debugOverlay->textDrawer.AppendText(fmt::format("Total downloaded: {0}\n", ByteToString(connectionInfo->peerInfo.totalByteReceived)));
					m_debugOverlay->textDrawer.AppendText(fmt::format("Total uploaded: {0}\n", ByteToString(connectionInfo->peerInfo.totalByteSent)));
					m_debugOverlay->textDrawer.AppendText(fmt::format("Packet loss: {0}%\n", 100ull * connectionInfo->peerInfo.totalPacketLost / connectionInfo->peerInfo.totalPacketSent));
				}
			}
		}

		// Raycast
		{
			auto& physSystem = stateData.world->GetSystem<Nz::Physics3DSystem>();
			Nz::Vector3f hitPos, hitNormal;
			if (physSystem.RaycastQuery(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 10.f, [&](const Nz::Physics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
				{
					//if (hitInfo.hitEntity != m_planetEntity)
					//  return std::nullopt;

					hitPos = hitInfo.hitPosition;
					hitNormal = hitInfo.hitNormal;
					return hitInfo.fraction;
				}))
			{
				debugDrawer.DrawLine(hitPos, hitPos + hitNormal * 0.2f, Nz::Color::Cyan());

				Nz::Vector3f blockPos = hitPos - hitNormal * m_planet->GetTileSize() * 0.25f;

				ChunkIndices chunkIndices = m_planet->GetChunkIndicesByPosition(blockPos);
				const Chunk* chunk = m_planet->GetChunk(chunkIndices);

				if (m_debugOverlay && m_debugOverlay->mode >= 1)
				{
					const ChunkIndices& chunkIndices = chunk->GetIndices();
					m_debugOverlay->textDrawer.AppendText(fmt::format("{0:-^{1}}\n", "Target", 20));
					m_debugOverlay->textDrawer.AppendText(fmt::format("Target chunk: {0};{1};{2}{3}\n", chunkIndices.x, chunkIndices.y, chunkIndices.z, chunk ? "" : " (not loaded)"));
				}

				if (chunk)
				{
					if (auto coordinates = chunk->ComputeCoordinates(blockPos))
					{
						if (m_debugOverlay && m_debugOverlay->mode >= 1)
						{
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target chunk block: {0};{1};{2}\n", coordinates->x, coordinates->y, coordinates->z));

							Nz::Vector3ui chunkCount(5);

							Nz::Vector3i maxHeight((Nz::Vector3i(chunkCount) + Nz::Vector3i(1)) / 2);
							maxHeight *= int(Planet::ChunkSize);

							const ChunkIndices& chunkIndices = chunk->GetIndices();
							Nz::Vector3i blockIndices = chunkIndices * int(Planet::ChunkSize) + Nz::Vector3i(coordinates->x, coordinates->z, coordinates->y) - Nz::Vector3i(int(Planet::ChunkSize)) / 2;
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target block global indices: {0};{1};{2}\n", blockIndices.x, blockIndices.y, blockIndices.z));
							unsigned int depth = Nz::SafeCaster(std::min({
								maxHeight.x - std::abs(blockIndices.x),
								maxHeight.y - std::abs(blockIndices.z),
								maxHeight.z - std::abs(blockIndices.y)
							}));
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target block depth: {0}\n", depth));
						}

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

		if (m_debugOverlay)
		{
			m_debugOverlay->label->UpdateText(m_debugOverlay->textDrawer);
			m_debugOverlay->label->Resize(m_debugOverlay->label->GetPreferredSize());
			m_debugOverlay->label->SetPosition({ 0.f, stateData.canvas->GetHeight() - m_debugOverlay->label->GetHeight() });
		}

		return true;
	}

	void GameState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_blockSelectionBar->Resize({ newSize.x, BlockSelectionBar::InventoryTileSize });
		m_blockSelectionBar->SetPosition({ 0.f, 5.f });

		m_chatBox->Resize(newSize);

		m_escapeMenu->Resize(m_escapeMenu->GetPreferredSize());
		m_escapeMenu->Center();
	}

	void GameState::OnTick(Nz::Time elapsedTime, bool lastTick)
	{
		AnimationSystem& animationSystem = GetStateData().world->GetSystem<AnimationSystem>();
		animationSystem.UpdateAnimationStates(elapsedTime);

		if (lastTick)
			SendInputs();
	}

	void GameState::SendInputs()
	{
		Packets::UpdatePlayerInputs inputPacket;

		inputPacket.inputs.index = m_nextInputIndex++;

		if (m_isMouseLocked)
		{
			inputPacket.inputs.crouch = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LControl);
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
		m_isMouseLocked = !m_chatBox->IsTyping() && !m_escapeMenu->IsVisible();
		Nz::Mouse::SetRelativeMouseMode(m_isMouseLocked);
	}
}
