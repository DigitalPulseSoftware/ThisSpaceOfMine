// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/GameState.hpp>
#include <ClientLib/BlockSelectionBar.hpp>
#include <ClientLib/Chatbox.hpp>
#include <ClientLib/Console.hpp>
#include <ClientLib/EscapeMenu.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <ClientLib/Components/ClientInteractibleComponent.hpp>
#include <ClientLib/Components/EnvironmentComponent.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <ClientLib/Systems/CameraFollowerSystem.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/PrimitiveList.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PipelinePassList.hpp>
#include <Nazara/Graphics/SpotLight.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Platform/Window.hpp>
#include <Nazara/Platform/WindowEventHandler.hpp>
#include <Nazara/TextRenderer/RichTextBuilder.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <Nazara/Widgets/SimpleLabelWidget.hpp>
#include <spdlog/spdlog.h>

#define DEBUG_ROTATION 0

namespace tsom
{
	GameState::GameState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_currentShipRotation(Nz::Quaternionf::Identity()),
	m_targetShipRotation(Nz::Quaternionf::Identity()),
	m_upCorrection(Nz::Quaternionf::Identity()),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_nextInputIndex(1),
	m_cameraMode(CameraMode::Firstperson),
	m_isMouseLocked(true)
	{
		auto& stateData = GetStateData();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();

#ifdef TSOM_DEV_TOOLS
			auto passList = filesystem.Load<Nz::PipelinePassList>(stateData.imgui ? "assets/3d_dev.passlist" : "assets/3d.passlist");
#else
			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");
#endif

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.EnableInfiniteZFar(true);
			cameraComponent.EnableReversedZ(true);
			cameraComponent.UpdateClearColor(Nz::Color::Black());
			cameraComponent.UpdateClearDepth(0.f);
			cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
			cameraComponent.UpdateZNear(0.1f);
			cameraComponent.UpdateZFar(10000.f); //< when infinite zfar is enabled, zfar is used as a limit for directional lights

			m_targetCameraFOV = cameraComponent.GetFOV();
		}
		m_environmentHandler.emplace(*stateData.app, *stateData.sessionHandler, *stateData.world, m_cameraEntity, *stateData.blockLibrary);

		m_crosshairEntity = CreateEntity();
		{
			auto sprite = std::make_shared<Nz::Sprite>(filesystem.Load<Nz::MaterialInstance>("assets/crosshair.png"));
			sprite->SetOrigin({ 0.5f, 0.5f });
			sprite->SetSize(sprite->GetSize() * 0.15f);

			m_crosshairEntity.emplace<Nz::NodeComponent>();
			auto& crosshairGfx = m_crosshairEntity.emplace<Nz::GraphicsComponent>(std::move(sprite), tsom::Constants::RenderMask2D);
			crosshairGfx.Hide();
		}

		m_healthOxygen.entity = CreateEntity();
		{
			m_healthOxygen.textSprite = std::make_shared<Nz::TextSprite>();

			m_healthOxygen.entity.emplace<Nz::NodeComponent>();
			m_healthOxygen.entity.emplace<Nz::GraphicsComponent>(m_healthOxygen.textSprite, tsom::Constants::RenderMask2D);
		}


		m_environmentHandler->OnControlledEntityStateUpdate.Connect([&](InputIndex inputIndex, const Packets::S_EntitiesStateUpdate::ControlledCharacter& characterStates)
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
				spdlog::debug("RECONCILIATION ERROR");
				m_predictedCameraRotation = Nz::EulerAnglesf(characterStates.cameraPitch, characterStates.cameraYaw, 0.f);
				spdlog::debug("Starting rotation: {0}", fmt::streamed(m_predictedCameraRotation));
				for (const InputRotation& predictedRotation : m_predictedInputRotations)
				{
					m_predictedCameraRotation.pitch = Nz::Clamp(m_predictedCameraRotation.pitch + predictedRotation.inputRotation.pitch, -89.f, 89.f);
					m_predictedCameraRotation.yaw += predictedRotation.inputRotation.yaw;
					m_predictedCameraRotation.Normalize();
					spdlog::debug("Adding {0} from input {1} which gives {2}", fmt::streamed(predictedRotation.inputRotation), predictedRotation.inputIndex, fmt::streamed(m_predictedCameraRotation));
				}
				spdlog::debug("Giving final rotation {0}", fmt::streamed(m_predictedCameraRotation));

				spdlog::debug("Error: {0}\n------", fmt::streamed(err));
			}
#endif
		});

		m_blockSelectionBar = CreateWidget<BlockSelectionBar>(*stateData.blockLibrary);

		m_controlledEntity = m_environmentHandler->GetControlledEntity();
		m_environmentHandler->OnControlledEntityChanged.Connect([&](entt::handle entity)
		{
			m_controlledEntity = entity;
			if (m_controlledEntity)
				BindControlledEntitySignals();
			else
				UpdateHealthAndOxygenText();
		});

		if (m_controlledEntity)
			BindControlledEntitySignals();
		else
			UpdateHealthAndOxygenText();

		m_mouseWheelMovedSlot.Connect(stateData.window->GetEventHandler().OnMouseWheelMoved, [&](const Nz::WindowEventHandler* /*eventHandler*/, const Nz::WindowEvent::MouseWheelEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			if (m_pilotedShip)
			{
				m_targetCameraDistance = Nz::Clamp(m_targetCameraDistance - event.delta, m_defaultCameraDistance * 0.5f, m_defaultCameraDistance * 2.f);
			}
			else if (m_blockSelectionBar->IsVisible())
			{
				if (event.delta < 0.f)
					m_blockSelectionBar->SelectNext();
				else
					m_blockSelectionBar->SelectPrevious();
			}
		});

		m_escapeMenu = CreateWidget<EscapeMenu>();
		m_escapeMenu->OnWidgetVisibilityUpdated.Connect([&](const Nz::BaseWidget* /*widget*/, bool /*isVisible*/)
		{
			UpdateMouseLock();
		});

		m_chatBox = CreateWidget<Chatbox>();
		m_chatBox->OnChatMessage.Connect([&](const std::string& message)
		{
			Packets::C_SendChatMessage messagePacket;
			messagePacket.message = message;

			stateData.networkSession->SendPacket(messagePacket);
		});
		m_chatBox->SetRenderLayerOffset(1);

		m_localConsole = CreateWidget<Console>();
		m_localConsole->SetBackgroundColor(Nz::Color(0.f, 0.f, 0.33f, 0.5f));
		m_localConsole->SetRenderLayerOffset(1);

		m_consoleExecutor.emplace(m_environmentHandler->GetScriptingContext());
		m_localConsole->OnCommand.Connect([this](std::string_view command)
		{
			m_consoleExecutor->Execute(command, "client console");
		});

		m_consoleExecutor->OnError.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
		{
			m_localConsole->PrintMessage(std::string(error), Nz::Color::Red());
		});

		m_consoleExecutor->OnOutput.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
		{
			m_localConsole->PrintMessage(std::string(error));
		});

		m_remoteConsole = CreateWidget<Console>();
		m_remoteConsole->SetBackgroundColor(Nz::Color(0.33f, 0.f, 0.f, 0.5f));
		m_remoteConsole->SetRenderLayerOffset(1);

		m_remoteConsole->OnCommand.Connect([this](std::string_view command)
		{
			Packets::C_SendConsoleCommand sendConsole;
			sendConsole.command = command;

			GetStateData().networkSession->SendPacket(sendConsole);
		});

		m_interactionLabel = CreateWidget<Nz::SimpleLabelWidget>();

		m_onUnhandledKeyPressed.Connect(stateData.canvas->OnUnhandledKeyPressed, [this](const Nz::WindowEventHandler*, const Nz::WindowEvent::KeyEvent& event)
		{
			auto& stateData = GetStateData();

			switch (event.scancode)
			{
				case Nz::Keyboard::Scancode::Tilde:
				{
					Console* targetConsole = (event.shift) ? m_remoteConsole : m_localConsole;

					if (targetConsole == m_remoteConsole)
					{
						if (m_localConsole->IsVisible())
							m_localConsole->Hide();
					}
					else if (targetConsole == m_localConsole)
					{
						if (m_remoteConsole->IsVisible())
							m_remoteConsole->Hide();
					}

					if (targetConsole->IsVisible())
						targetConsole->Hide();
					else
					{
						targetConsole->Show();

						// Execute next update to avoid the following TextEntered to be sent to the console
						m_timerManager.AddImmediateTimer([this, targetConsole]
						{
							if (targetConsole->IsVisible())
								targetConsole->SetFocus();
						});
					}
					UpdateMouseLock();

					break;
				}

				case Nz::Keyboard::Scancode::Escape:
				{
					if (m_escapeMenu->IsVisible())
						m_escapeMenu->Hide();
					else if (m_localConsole->IsVisible())
						m_localConsole->Hide();
					else if (m_remoteConsole->IsVisible())
						m_remoteConsole->Hide();
					else if (m_chatBox->IsOpen())
						m_chatBox->Close();
					else if (m_pilotedShip)
						stateData.networkSession->SendPacket(Packets::C_ExitShipControl{});
					else
						m_escapeMenu->Show();

					UpdateMouseLock();
					break;
				}

				case Nz::Keyboard::Scancode::Return:
				case Nz::Keyboard::Scancode::NumpadReturn:
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

				case Nz::Keyboard::Scancode::E:
				{
					if (auto raycastHit = RaycastQuery())
					{
						if (auto* interactible = raycastHit->hitEntity.try_get<ClientInteractibleComponent>(); interactible && interactible->isEnabled)
						{
							auto& entityNetId = raycastHit->hitEntity.get<ClientEntityNetworkIndex>();

							Packets::C_Interact interact;
							interact.entityId = entityNetId.networkIndex;

							stateData.networkSession->SendPacket(interact);
						}
					}
					break;
				}

				case Nz::Keyboard::Scancode::F2:
				{
					auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

					auto primitive = Nz::Primitive::IcoSphere(1.f, 2);

					auto collider = Nz::Collider3D::Build(primitive);

					Nz::RigidBody3D::DynamicSettings dynSettings(collider, 10.f);
					dynSettings.allowSleeping = false;

					entt::handle debugEntity = stateData.world->CreateEntity();
					//debugEntity.emplace<PlanetComponent>().planet = m_planet.get();
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

				case Nz::Keyboard::Scancode::F3:
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

				case Nz::Keyboard::Scancode::F4:
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

				case Nz::Keyboard::Scancode::F5:
				{
					if (m_cameraMode >= CameraMode::Last)
						m_cameraMode = CameraMode::First;
					else
						m_cameraMode = static_cast<CameraMode>(Nz::UnderlyingCast(m_cameraMode) + 1);

					auto& cameraComponent = m_cameraEntity.get<Nz::CameraComponent>();
					if (m_cameraMode != CameraMode::Firstperson)
						cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D);
					else
						cameraComponent.UpdateRenderMask(tsom::Constants::RenderMask3D & ~tsom::Constants::RenderMaskLocalPlayer);
					break;
				}

				default:
					break;
			}
		});

		m_onChatMessage.Connect(stateData.sessionHandler->OnChatMessage, [this](const std::string& message)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::TextItem{ message } }
				}
			});

			spdlog::info("{0}", message);
		});

		m_onConsoleOutput.Connect(stateData.sessionHandler->OnConsoleOutput, [this](const Nz::Color& color, std::string_view message)
		{
			if (m_remoteConsole)
				m_remoteConsole->PrintMessage(std::string(message), color);
		});

		m_onPlayerChatMessage.Connect(stateData.sessionHandler->OnPlayerChatMessage, [this](const std::string& message, const ClientSessionHandler::PlayerInfo& playerInfo)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::ColorItem{ (playerInfo.isAuthenticated) ? Nz::Color::Yellow() : Nz::Color::Gray() }},
					{ Chatbox::TextItem{ playerInfo.nickname } },
					{ Chatbox::TextItem{ ": " }},
					{ Chatbox::ColorItem{ Nz::Color::White() } },
					{ Chatbox::TextItem{ message } }
				}
			});

			spdlog::info("{0}: {1}", playerInfo.nickname, message);
		});

		m_onPlayerJoined.Connect(stateData.sessionHandler->OnPlayerJoined, [this](const ClientSessionHandler::PlayerInfo& playerInfo)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::ColorItem{ (playerInfo.isAuthenticated) ? Nz::Color::Yellow() : Nz::Color::Gray() }},
					{ Chatbox::TextItem{ playerInfo.nickname } },
					{ Chatbox::ColorItem{ Nz::Color::White() } },
					{ Chatbox::TextItem{ " joined the server" } }
				}
			});

			spdlog::info("{0} joined the server", playerInfo.nickname);
		});

		m_onPlayerLeave.Connect(stateData.sessionHandler->OnPlayerLeave, [this](const ClientSessionHandler::PlayerInfo& playerInfo)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::ColorItem{ (playerInfo.isAuthenticated) ? Nz::Color::Yellow() : Nz::Color::Gray() }},
					{ Chatbox::TextItem{ playerInfo.nickname } },
					{ Chatbox::ColorItem{ Nz::Color::White() } },
					{ Chatbox::TextItem{ " left the server" } }
				}
			});

			spdlog::info("{0} left the server", playerInfo.nickname);
		});

		m_onPlayerNameUpdate.Connect(stateData.sessionHandler->OnPlayerNameUpdate, [this](const ClientSessionHandler::PlayerInfo& playerInfo, const std::string& newNickname)
		{
			m_chatBox->PrintMessage({
				{
					{ Chatbox::ColorItem{ (playerInfo.isAuthenticated) ? Nz::Color::Yellow() : Nz::Color::Gray() }},
					{ Chatbox::TextItem{ playerInfo.nickname } },
					{ Chatbox::ColorItem{ Nz::Color::White() } },
					{ Chatbox::TextItem{ " changed their nickname to " } },
					{ Chatbox::ColorItem{ (playerInfo.isAuthenticated) ? Nz::Color::Yellow() : Nz::Color::Gray() } },
					{ Chatbox::TextItem{ newNickname } }
				}
			});

			spdlog::info("{0} renamed to {1}", playerInfo.nickname, newNickname);
		});

		m_environmentHandler->OnPilotShip.Connect([this](entt::handle shipEntity, entt::handle shipExteriorEntity, const Nz::Quaternionf& referenceRotation)
		{
			m_pilotedShip = PilotedShip{
				.exteriorEntity = shipExteriorEntity,
				.interiorEntity = shipEntity
			};

			bool isControllingShip = static_cast<bool>(shipExteriorEntity);

			m_blockSelectionBar->Show(!isControllingShip);
			m_crosshairEntity.get<Nz::GraphicsComponent>().Show(isControllingShip);

			m_shipAABB = Nz::Boxf::Invalid();
			m_shipReferenceRotation = referenceRotation;

			if (auto* shipComponent = shipEntity.try_get<ShipComponent>())
			{
				for (auto& chunkEntitiesPtr : shipComponent->shipEntities)
				{
					if (!chunkEntitiesPtr)
						continue;

					chunkEntitiesPtr->ForEachChunk([&](const ChunkIndices& chunkIndices, entt::handle chunkEntity)
					{
						auto& chunkGfx = chunkEntity.get<Nz::GraphicsComponent>();
						if (m_shipAABB.IsValid())
							m_shipAABB.ExtendTo(chunkGfx.GetAABB());
						else
							m_shipAABB = chunkGfx.GetAABB();
					});
				}
			}

			m_targetCameraDistance = m_currentCameraDistance = m_defaultCameraDistance = m_shipAABB.GetRadius();
			LayoutWidgets(Nz::Vector2f(GetStateData().renderTarget->GetSize()));
		});

		m_environmentHandler->OnStopPilotingShip.Connect([this]
		{
			m_pilotedShip = {};
			m_blockSelectionBar->Show();
			m_crosshairEntity.get<Nz::GraphicsComponent>().Show(false);
			LayoutWidgets(Nz::Vector2f(GetStateData().renderTarget->GetSize()));
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
		m_localConsole->Hide();
		m_remoteConsole->Hide();

		auto& stateData = GetStateData();
		LayoutWidgets(Nz::Vector2f(stateData.renderTarget->GetSize()));

		auto& config = stateData.app->GetComponent<GameConfigAppComponent>().GetConfig();

		m_incomingCameraRotation = Nz::EulerAnglesf::Zero();
		m_remainingCameraRotation = Nz::EulerAnglesf::Zero();
		m_predictedCameraRotation = Nz::EulerAnglesf::Zero();

		float mouseSensitivity = config.GetFloatValue<float>("Input.MouseSensitivity");
		m_mouseMovedSlot.Connect(stateData.canvas->OnUnhandledMouseMoved, [&, mouseSensitivity](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event)
		{
			if (!m_isMouseLocked)
				return;

			auto& stateData = GetStateData();

			float pitchMod = -event.deltaY * mouseSensitivity;
			float yawMod = -event.deltaX * mouseSensitivity;

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

			if (!m_pilotedShip)
			{
				if (auto raycastHit = RaycastQuery())
				{
					if (auto* chunkComponent = raycastHit->hitEntity.try_get<ChunkComponent>())
					{
						auto& chunkNetworkMap = chunkComponent->parentEntity.get<ChunkNetworkMapComponent>();
						auto& chunkRigidBody = raycastHit->hitEntity.get<Nz::RigidBody3DComponent>();
						auto& chunkNode = raycastHit->hitEntity.get<Nz::NodeComponent>();

						const Chunk& hitChunk = *chunkComponent->chunk;
						const ChunkContainer& chunkContainer = hitChunk.GetContainer();

						Nz::Vector3f localPos = chunkNode.ToLocalPosition(raycastHit->hitPos);
						Nz::Vector3f localNormal = chunkNode.ToLocalDirection(raycastHit->hitNormal);

						auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), raycastHit->subShapeID);
						if (!hitCoordinates)
							return;

						if (event.button == Nz::Mouse::Left)
						{
							// Mine
							Packets::C_MineBlock mineBlock;
							mineBlock.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, &hitChunk);
							mineBlock.voxelLoc.x = hitCoordinates->blockIndices.x;
							mineBlock.voxelLoc.y = hitCoordinates->blockIndices.y;
							mineBlock.voxelLoc.z = hitCoordinates->blockIndices.z;

							stateData.networkSession->SendPacket(mineBlock);
						}
						else
						{
							BlockIndices blockIndices = chunkContainer.GetBlockIndices(hitChunk.GetIndices(), hitCoordinates->blockIndices);

							const DirectionAxis& dirAxis = s_dirAxis[hitCoordinates->direction];

							blockIndices[dirAxis.upAxis] += dirAxis.upDir;

							Nz::Vector3ui innerCoordinates;
							ChunkIndices chunkIndices = chunkContainer.GetChunkIndicesByBlockIndices(blockIndices, &innerCoordinates);
							const Chunk* chunk = chunkContainer.GetChunk(chunkIndices);
							if (!chunk)
								return;

							Packets::C_PlaceBlock placeBlock;
							placeBlock.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, chunk);
							placeBlock.voxelLoc.x = innerCoordinates.x;
							placeBlock.voxelLoc.y = innerCoordinates.y;
							placeBlock.voxelLoc.z = innerCoordinates.z;
							placeBlock.newContent = Nz::SafeCast<Nz::UInt8>(m_blockSelectionBar->GetSelectedBlock());

							stateData.networkSession->SendPacket(placeBlock);
						}
					}
				}
			}
		});

		UpdateMouseLock();
	}

	void GameState::Leave(Nz::StateMachine& fsm)
	{
		WidgetState::Leave(fsm);

		if (Nz::Window* window = GetStateData().window)
			window->SetRelativeMouseMode(false);

		m_chatBox->Close();
	}

	bool GameState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!WidgetState::Update(fsm, elapsedTime))
			return false;

		auto& stateData = GetStateData();
		if (!stateData.networkSession)
			return true;

#ifdef TSOM_DEV_TOOLS
		m_environmentHandler->Update(elapsedTime, stateData.imgui);
#else
		m_environmentHandler->Update(elapsedTime, nullptr);
#endif

		m_timerManager.Update(elapsedTime);
		m_chatBox->Update();

		if (m_debugOverlay)
			m_debugOverlay->textDrawer.Clear();

		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			m_tickAccumulator -= m_tickDuration;
			OnTick(m_tickDuration, m_tickAccumulator < m_tickDuration);
		}

		float cameraSpeed = (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift)) ? 50.f : 10.f;
		float updateTime = elapsedTime.AsSeconds();

		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();

		Nz::DebugDrawer* debugDrawer = m_cameraEntity.get<Nz::CameraComponent>().AccessDebugDrawer();
		m_environmentHandler->Draw(elapsedTime, debugDrawer);

		if (m_controlledEntity)
		{
			Nz::NodeComponent& characterNode = m_controlledEntity.get<Nz::NodeComponent>();

			Nz::Vector3f characterPos = characterNode.GetGlobalPosition();
			Nz::Quaternionf characterRot = characterNode.GetGlobalRotation();

			Nz::EulerAnglesf predictedCameraRotation = m_predictedCameraRotation;
			if (!m_pilotedShip)
			{
				predictedCameraRotation.pitch = Nz::Clamp(predictedCameraRotation.pitch + m_incomingCameraRotation.pitch, -89.f, 89.f);
				predictedCameraRotation.yaw += m_incomingCameraRotation.yaw;
				predictedCameraRotation.Normalize();
			}

			switch (m_cameraMode)
			{
				case CameraMode::Firstperson:
				{
					const Nz::Node* environmentNode = characterNode.GetParent();
					if NAZARA_UNLIKELY(!environmentNode)
					{
						spdlog::error("character has no environment node");
						break;
					}

					cameraNode.SetPosition(characterPos + characterRot * (Nz::Vector3f::Up() * Constants::PlayerCameraHeight));

					Nz::Quaternionf cameraRotation = environmentNode->GetGlobalRotation() * m_referenceRotation * Nz::Quaternionf(predictedCameraRotation);
					cameraRotation.Normalize();

					if (m_pilotedShip)
						cameraRotation *= m_currentShipRotation;

					cameraNode.SetRotation(cameraRotation);
					break;
				}

				case CameraMode::Thirdperson:
				case CameraMode::ThirdpersonRear:
				{
					Nz::Vector3f sourceCameraPos;
					Nz::Vector3f targetCameraPos;
					Nz::Quaternionf targetCameraRot;

					if (m_pilotedShip)
					{
						auto& shipNode = m_pilotedShip->exteriorEntity.get<Nz::NodeComponent>();

						Nz::Quaternionf forwardRotation = m_shipReferenceRotation;
						if (m_cameraMode == CameraMode::ThirdpersonRear)
							forwardRotation *= Nz::Quaternionf(Nz::TurnAnglef(0.5f), Nz::Vector3f::Up());

						Nz::Quaternionf cameraRotation = shipNode.GetGlobalRotation() * forwardRotation;
						cameraRotation *= m_currentShipRotation;
						cameraRotation.Normalize();

						m_currentCameraDistance = Nz::Lerp(m_currentCameraDistance, m_targetCameraDistance, 2.f * updateTime);

						sourceCameraPos = shipNode.ToGlobalPosition(m_shipAABB.GetCenter());

						targetCameraPos = sourceCameraPos + cameraRotation * (Nz::Vector3f::Backward() * 2.f + Nz::Vector3f::Up()) * m_currentCameraDistance;
						targetCameraRot = cameraRotation * Nz::Quaternionf(Nz::DegreeAnglef(-5.f), Nz::Vector3f::Right());
					}
					else
					{
						sourceCameraPos = characterPos;

						targetCameraRot = characterRot * Nz::EulerAnglesf(predictedCameraRotation.pitch, (m_cameraMode == CameraMode::Thirdperson) ? 0.f : 180.f, 0.f);
						targetCameraRot.Normalize();

						targetCameraPos = characterPos + characterRot * (Nz::Vector3f::Up() * 1.f) + targetCameraRot * Nz::Vector3f::Backward() * 1.f;
					}

					cameraNode.SetPosition(RaycastCamera(sourceCameraPos, targetCameraPos));
					cameraNode.SetRotation(targetCameraRot);
					break;
				}

				default:
					break;
			}

			auto& cameraComponent = m_cameraEntity.get<Nz::CameraComponent>();
			Nz::DegreeAnglef cameraFov = cameraComponent.GetFOV();
			if (!cameraFov.ApproxEqual(m_targetCameraFOV))
			{
				float factor = (m_targetCameraFOV == Constants::DefaultCameraFOV) ? 2.f * updateTime : 0.5f * updateTime;
				cameraComponent.UpdateFOV(Nz::Lerp(cameraFov, m_targetCameraFOV, factor));
			}

			if (m_debugOverlay)
			{
				Nz::Vector3f localPos = characterNode.GetPosition();
				Nz::Quaternionf localRot = characterNode.GetRotation();

				m_debugOverlay->textDrawer.AppendText(fmt::format("{0:-^{1}}\n", "Player position", 20));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Position: {0:.3f};{1:.3f};{2:.3f} (local: {3:.3f};{4:.3f};{5:.3f})\n", characterPos.x, characterPos.y, characterPos.z, localPos.x, localPos.y, localPos.z));
				m_debugOverlay->textDrawer.AppendText(fmt::format("Rotation: {0:.3f};{1:.3f};{2:.3f};{3:.3f} (local: {4:.3f};{5:.3f};{6:.3f};{7:.3f})\n", characterRot.x, characterRot.y, characterRot.z, characterRot.w, localRot.x, localRot.y, localRot.z, localRot.w));

				if (m_debugOverlay->mode >= 1)
				{
					EnvironmentComponent& characterEnv = m_controlledEntity.get<EnvironmentComponent>();

					m_debugOverlay->textDrawer.AppendText(fmt::format("Current environment: #{}\n", characterEnv.environmentIndex));
					if (const GravityController* gravityController = m_environmentHandler->GetGravityController(characterEnv.environmentIndex))
					{
						GravityForce gravityForce = gravityController->ComputeGravity(localPos);
						m_debugOverlay->textDrawer.AppendText(fmt::format("Current gravity: {0} x {1:.3f};{2:.3f};{3:.3f} ({4:.2f}%)\n", gravityForce.acceleration, gravityForce.direction.x, gravityForce.direction.y, gravityForce.direction.z, gravityForce.factor * 100.f));
					}
				}

				/*Nz::Vector3f up = m_planet->ComputeUpDirection(characterPos);
				float gravity = m_planet->GetGravityFactor(characterPos);

				m_debugOverlay->textDrawer.AppendText(fmt::format("Up direction: {0:.3f};{1:.3f};{2:.3f} - gravity: {3:.2f}\n", up.x, up.y, up.z, gravity));

				ChunkIndices chunkIndices = m_planet->GetChunkIndicesByPosition(characterPos);
				const Chunk* chunk = m_planet->GetChunk(chunkIndices);

				m_debugOverlay->textDrawer.AppendText(fmt::format("Chunk: {0};{1};{2}{3}\n", chunkIndices.x, chunkIndices.y, chunkIndices.z, chunk ? "" : " (not loaded)"));

				if (const Chunk* chunk = m_planet->GetChunk(chunkIndices))
				{
					if (auto coordinates = chunk->ComputeCoordinates(characterPos))
						m_debugOverlay->textDrawer.AppendText(fmt::format("Chunk block: {0};{1};{2}\n", coordinates->x, coordinates->y, coordinates->z));
				}*/
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
		entt::handle interactibleEntity;
		if (m_cameraMode != CameraMode::ThirdpersonRear && !m_pilotedShip)
		{
			if (auto raycastHit = RaycastQuery())
			{
				if (auto* chunkComponent = raycastHit->hitEntity.try_get<ChunkComponent>())
				{
					auto& chunkRigidBody = raycastHit->hitEntity.get<Nz::RigidBody3DComponent>();
					auto& chunkNode = raycastHit->hitEntity.get<Nz::NodeComponent>();

					const Chunk& hitChunk = *chunkComponent->chunk;
					const ChunkContainer& chunkContainer = hitChunk.GetContainer();

					debugDrawer->DrawLine(raycastHit->hitPos, raycastHit->hitPos + raycastHit->hitNormal * 0.2f, Nz::Color::Cyan());

					if (m_debugOverlay && m_debugOverlay->mode >= 1)
					{
						const ChunkIndices& chunkIndices = hitChunk.GetIndices();
						m_debugOverlay->textDrawer.AppendText(fmt::format("{0:-^{1}}\n", "Target", 20));
						m_debugOverlay->textDrawer.AppendText(fmt::format("Target chunk: {0};{1};{2}\n", chunkIndices.x, chunkIndices.y, chunkIndices.z));
					}

					Nz::Vector3f localPos = chunkNode.ToLocalPosition(raycastHit->hitPos);
					Nz::Vector3f localNormal = chunkNode.ToLocalDirection(raycastHit->hitNormal);

					auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), raycastHit->subShapeID);
					if (hitCoordinates)
					{
						if (m_debugOverlay && m_debugOverlay->mode >= 1)
						{
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target chunk block: {0};{1};{2}\n", hitCoordinates->blockIndices.x, hitCoordinates->blockIndices.y, hitCoordinates->blockIndices.z));

							Nz::Vector3ui chunkCount(5);

							Nz::Vector3i maxHeight((Nz::Vector3i(chunkCount) + Nz::Vector3i(1)) / 2);
							maxHeight *= int(Planet::ChunkSize);

							const ChunkIndices& chunkIndices = hitChunk.GetIndices();
							Nz::Vector3i blockIndices = chunkIndices * int(Planet::ChunkSize) + Nz::Vector3i(hitCoordinates->blockIndices.x, hitCoordinates->blockIndices.z, hitCoordinates->blockIndices.y) - Nz::Vector3i(int(Planet::ChunkSize)) / 2;
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target block global indices: {0};{1};{2}\n", blockIndices.x, blockIndices.y, blockIndices.z));
							unsigned int depth = Nz::SafeCaster(std::min({
								maxHeight.x - std::abs(blockIndices.x),
								maxHeight.y - std::abs(blockIndices.z),
								maxHeight.z - std::abs(blockIndices.y)
							}));
							m_debugOverlay->textDrawer.AppendText(fmt::format("Target block depth: {0}\n", depth));
						}

						auto cornerPos = hitChunk.ComputeBlockCorners(hitCoordinates->blockIndices);

						constexpr Nz::EnumArray<Direction, std::array<Nz::BoxCorner, 4>> directionToCorners = {
							// Back
							std::array{ Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::LeftTopNear },
							// Down
							std::array{ Nz::BoxCorner::LeftBottomFar, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::RightTopFar, Nz::BoxCorner::LeftTopFar },
							// Front
							std::array{ Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::RightTopNear, Nz::BoxCorner::RightTopFar },
							// Left
							std::array{ Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::RightBottomFar, Nz::BoxCorner::LeftBottomFar },
							// Right
							std::array{ Nz::BoxCorner::RightTopNear, Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::LeftTopFar, Nz::BoxCorner::RightTopFar },
							// Up
							std::array{ Nz::BoxCorner::RightBottomNear, Nz::BoxCorner::LeftBottomNear, Nz::BoxCorner::LeftTopNear, Nz::BoxCorner::RightTopNear },
						};

						auto& corners = directionToCorners[hitCoordinates->direction];

						debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[0]]), chunkNode.ToGlobalPosition(cornerPos[corners[1]]), Nz::Color::Green());
						debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[1]]), chunkNode.ToGlobalPosition(cornerPos[corners[2]]), Nz::Color::Green());
						debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[2]]), chunkNode.ToGlobalPosition(cornerPos[corners[3]]), Nz::Color::Green());
						debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[3]]), chunkNode.ToGlobalPosition(cornerPos[corners[0]]), Nz::Color::Green());
					}
				}
				else if (auto* interactible = raycastHit->hitEntity.try_get<ClientInteractibleComponent>(); interactible && interactible->isEnabled)
				{
					interactibleEntity = raycastHit->hitEntity;
				}
			}
		}

		if (interactibleEntity)
		{
			auto& interactible = interactibleEntity.get<ClientInteractibleComponent>();
			m_interactionLabel->SetCharacterSize(36);
			m_interactionLabel->SetText(fmt::format("{} ({})", (!interactible.interactText.empty()) ? interactible.interactText : "Use", "E"));
			m_interactionLabel->Center();
			m_interactionLabel->Show();
		}
		else
			m_interactionLabel->Hide();

		m_controlledEntity = m_environmentHandler->GetControlledEntity();

		if (m_debugOverlay)
		{
			m_debugOverlay->label->UpdateText(m_debugOverlay->textDrawer);
			m_debugOverlay->label->SetPosition({ 0.f, stateData.canvas->GetHeight() - m_debugOverlay->label->GetHeight() });
		}

		if (m_pilotedShip)
		{
			float factor = 2.f * updateTime;
			m_currentShipRotation = Nz::Quaternionf::Slerp(m_currentShipRotation, m_targetShipRotation, factor);
		}

		GetStateData().world->GetSystem<CameraFollowerSystem>().SetCameraPosition(m_cameraEntity.get<Nz::NodeComponent>().GetGlobalPosition());

		return true;
	}

	void GameState::BindControlledEntitySignals()
	{
		auto& entityInstance = m_controlledEntity.get<ClassInstanceComponent>();

		// TODO: Cache those
		Nz::UInt32 healthIndex = entityInstance.GetClass()->FindProperty("health");
		Nz::UInt32 oxygenIndex = entityInstance.GetClass()->FindProperty("oxygen");

		m_controlledEntityPropertyUpdate.Connect(entityInstance.OnPropertyUpdate, [this, healthIndex, oxygenIndex](ClassInstanceComponent* /*classInstance*/, Nz::UInt32 propertyIndex, const EntityProperty& /*newValue*/)
		{
			if (propertyIndex == healthIndex || propertyIndex == oxygenIndex)
			{
				// Update next tick when property value has been updated
				m_timerManager.AddImmediateTimer([this] { UpdateHealthAndOxygenText(); });
			}
		});

		UpdateHealthAndOxygenText();
	}

	void GameState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		float hudNextHeight = 0.f;
		if (m_blockSelectionBar->IsVisible())
		{
			m_blockSelectionBar->Resize({ newSize.x, BlockSelectionBar::InventoryTileSize });
			m_blockSelectionBar->SetPosition({ 0.f, 5.f });

			hudNextHeight += m_blockSelectionBar->GetPosition().y + m_blockSelectionBar->GetHeight();
		}

		m_chatBox->Resize(newSize);

		for (Console* console : { m_localConsole, m_remoteConsole })
		{
			console->Resize({ newSize.x, newSize.y / 3.f });
			console->SetPosition({ 0.f, newSize.y - console->GetHeight() });
		}

		m_crosshairEntity.get<Nz::NodeComponent>().SetPosition({ newSize.x * 0.5f, newSize.y * 0.5f });
		m_healthOxygen.entity.get<Nz::NodeComponent>().SetPosition({ newSize.x * 0.5f - m_healthOxygen.entity.get<Nz::GraphicsComponent>().GetAABB().x / 2.f, hudNextHeight });

		m_escapeMenu->Center();
	}

	Nz::Vector3f GameState::RaycastCamera(const Nz::Vector3f& from, const Nz::Vector3f& to)
	{
		auto& physSystem = GetStateData().world->GetSystem<Nz::Physics3DSystem>();

		Nz::Vector3f targetPos = to;
		auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
		{
			targetPos = hitInfo.hitPosition + hitInfo.hitNormal * 0.2f;
		};

		struct OnlyStatic : Nz::PhysBroadphaseLayerFilter3D
		{
			bool ShouldCollide(Nz::PhysBroadphase3D layer) const override
			{
				return layer == Constants::BroadphaseStatic;
			}
		};

		struct IgnoreShip : Nz::PhysBodyFilter3D
		{
			bool ShouldCollide(Nz::UInt32 bodyIndex) const override
			{
				entt::handle entity = physicsSystem->GetRigidBodyEntity(bodyIndex);
				if (!entity)
					return false;

				ChunkComponent* chunkComponent = entity.try_get<ChunkComponent>();
				if (!chunkComponent)
					return false;

				return chunkComponent->parentEntity != shipInteriorEntity;
			}

			bool ShouldCollideLocked(const Nz::PhysBody3D& body) const override
			{
				return ShouldCollide(body.GetBodyIndex());
			}

			Nz::Physics3DSystem* physicsSystem;
			entt::handle shipInteriorEntity;
		};

		OnlyStatic onlyStatic;
		IgnoreShip ignoreShip;
		if (m_pilotedShip)
		{
			ignoreShip.shipInteriorEntity = m_pilotedShip->interiorEntity;
			ignoreShip.physicsSystem = &physSystem;
		}

		physSystem.RaycastQueryFirst(from, to, callback, &onlyStatic, nullptr, (m_pilotedShip) ? &ignoreShip : nullptr);

		return targetPos;
	}

	auto GameState::RaycastQuery() const -> std::optional<RaycastResult>
	{
		auto& physSystem = GetStateData().world->GetSystem<Nz::Physics3DSystem>();

		RaycastResult raycastResult;
		auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
		{
			raycastResult.hitEntity = hitInfo.hitEntity;
			raycastResult.hitPos = hitInfo.hitPosition;
			raycastResult.hitNormal = hitInfo.hitNormal;
			raycastResult.subShapeID = hitInfo.subShapeID;
		};

		struct IgnoreSelf : Nz::PhysBodyFilter3D
		{
			bool ShouldCollide(Nz::UInt32 bodyIndex) const override
			{
				return playerBodyIndex != bodyIndex;
			}

			bool ShouldCollideLocked(const Nz::PhysBody3D& body) const override
			{
				return body.GetBodyIndex() != playerBodyIndex;
			}

			Nz::UInt32 playerBodyIndex;
		};

		IgnoreSelf ignoreSelf;
		if (m_controlledEntity)
			ignoreSelf.playerBodyIndex = m_controlledEntity.get<Nz::RigidBody3DComponent>().GetBodyIndex();

		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		if (!physSystem.RaycastQueryFirst(cameraNode.GetPosition(), cameraNode.GetPosition() + cameraNode.GetForward() * 10.f, callback, nullptr, nullptr, (m_controlledEntity) ? &ignoreSelf : nullptr))
			return {};

		return raycastResult;
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
		Packets::C_UpdatePlayerInputs inputPacket;
		inputPacket.inputs.index = m_nextInputIndex++;

		if (m_isMouseLocked)
		{
			if (m_pilotedShip && !Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LAlt))
			{
				PlayerInputs::Ship& shipInputs = inputPacket.inputs.data.emplace<PlayerInputs::Ship>();
				shipInputs.moveForward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W);
				shipInputs.moveBackward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S);
				shipInputs.moveLeft = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A);
				shipInputs.moveRight = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D);
				shipInputs.moveUp = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space);
				shipInputs.moveDown = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LControl);
				shipInputs.rollLeft = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::Q);
				shipInputs.rollRight = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::E);
				shipInputs.stabilize = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::R);
				shipInputs.pitch = m_incomingCameraRotation.pitch;
				shipInputs.yaw = m_incomingCameraRotation.yaw;

				// TODO: Use ship acceleration instead
				m_targetCameraFOV = Constants::DefaultCameraFOV;
				if (shipInputs.moveForward)
					m_targetCameraFOV = Nz::DegreeAnglef(110.f);
				else if (shipInputs.moveBackward)
					m_targetCameraFOV = Nz::DegreeAnglef(80.f);

				float roll = 0.f;
				if (shipInputs.rollLeft)
					roll += 10.f;
				if (shipInputs.rollRight)
					roll -= 10.f;

				m_targetShipRotation = Nz::EulerAnglesf(shipInputs.pitch, shipInputs.yaw, roll);
			}
			else
			{
				PlayerInputs::Character& characterInputs = inputPacket.inputs.data.emplace<PlayerInputs::Character>();
				if (!m_pilotedShip)
				{
					characterInputs.crouch = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::LControl);
					characterInputs.jump = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space);
					characterInputs.moveForward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::W);
					characterInputs.moveBackward = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::S);
					characterInputs.moveLeft = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::A);
					characterInputs.moveRight = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::Scancode::D);
					characterInputs.sprint = Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift);
				}

				if (m_controlledEntity)
				{
					m_remainingCameraRotation.pitch += m_incomingCameraRotation.pitch;
					m_remainingCameraRotation.yaw += m_incomingCameraRotation.yaw;

					if (!m_remainingCameraRotation.pitch.ApproxEqual(Nz::DegreeAnglef::Zero()) || !m_remainingCameraRotation.yaw.ApproxEqual(Nz::DegreeAnglef::Zero()))
					{
						Nz::DegreeAnglef inputPitch = Nz::DegreeAnglef::Clamp(m_remainingCameraRotation.pitch, -Constants::PlayerRotationSpeed, Constants::PlayerRotationSpeed);
						Nz::DegreeAnglef inputYaw = Nz::DegreeAnglef::Clamp(m_remainingCameraRotation.yaw, -Constants::PlayerRotationSpeed, Constants::PlayerRotationSpeed);

						characterInputs.pitch = inputPitch;
						characterInputs.yaw = inputYaw;

						m_remainingCameraRotation.pitch -= inputPitch;
						m_remainingCameraRotation.yaw -= inputYaw;

						m_predictedCameraRotation.pitch = Nz::Clamp(m_predictedCameraRotation.pitch + inputPitch, -89.f, 89.f);
						m_predictedCameraRotation.yaw += inputYaw;
						m_predictedCameraRotation.Normalize();

						m_predictedInputRotations.push_back({
							.inputIndex = inputPacket.inputs.index,
							.inputRotation = Nz::EulerAnglesf(characterInputs.pitch, characterInputs.yaw, Nz::DegreeAnglef::Zero())
						});
					}
				}
			}

			m_incomingCameraRotation.pitch = Nz::DegreeAnglef::Zero();
			m_incomingCameraRotation.yaw = Nz::DegreeAnglef::Zero();
		}

		GetStateData().networkSession->SendPacket(inputPacket);
	}

	void GameState::UpdateHealthAndOxygenText()
	{
		Nz::Vector2f size = Nz::Vector2f(GetStateData().renderTarget->GetSize());

		m_healthOxygen.textDrawer.Clear();
		Nz::RichTextBuilder richTextBuilder(m_healthOxygen.textDrawer);
		richTextBuilder << richTextBuilder.CharacterSize(18);

		if (m_controlledEntity)
		{
			auto& playerInstance = m_controlledEntity.get<ClassInstanceComponent>();

			// TODO: cache those
			Nz::UInt32 healthPropertyIndex = playerInstance.FindPropertyIndex("health");
			Nz::UInt32 oxygenPropertyIndex = playerInstance.FindPropertyIndex("oxygen");

			auto healthValue = std::get<EntityPropertySingleValue<EntityPropertyType::Integer>>(playerInstance.GetProperty(healthPropertyIndex));
			auto oxygenValue = std::get<EntityPropertySingleValue<EntityPropertyType::Integer>>(playerInstance.GetProperty(oxygenPropertyIndex));

			richTextBuilder << Nz::Color::Yellow() << "Status: " << Nz::Color::White() << fmt::format("{}%", *healthValue);
			richTextBuilder << Nz::Color::White() << " - ";
			richTextBuilder << Nz::Color::Cyan() << "Oxygen: " << Nz::Color::White() << fmt::format("{}%", *oxygenValue);
		}
		else
		{
			richTextBuilder << Nz::Color::Yellow() << "Status: ";
			richTextBuilder << Nz::Color::Red() << "Dead";
		}

		m_healthOxygen.textSprite->Update(m_healthOxygen.textDrawer);

		m_healthOxygen.entity.get<Nz::NodeComponent>().SetPosition({ size.x * 0.5f - m_healthOxygen.entity.get<Nz::GraphicsComponent>().GetAABB().x / 2.f, m_blockSelectionBar->GetPosition().y + m_blockSelectionBar->GetHeight() });
	}

	void GameState::UpdateMouseLock()
	{
		m_isMouseLocked = !m_chatBox->IsTyping() && !m_escapeMenu->IsVisible() && !m_localConsole->IsVisible() && !m_remoteConsole->IsVisible();
		m_chatBox->EnableMouseInput(!m_isMouseLocked);
		m_localConsole->EnableMouseInput(!m_isMouseLocked);
		m_remoteConsole->EnableMouseInput(!m_isMouseLocked);

		if (Nz::Window* window = GetStateData().window)
			window->SetRelativeMouseMode(m_isMouseLocked);
	}
}
