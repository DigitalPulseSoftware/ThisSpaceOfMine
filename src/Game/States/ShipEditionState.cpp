// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/ShipEditionState.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Primitive.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/FramePipeline.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/Components/CameraComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Graphics/Systems/RenderSystem.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <Nazara/Platform/Window.hpp>
#include <Nazara/TextRenderer/RichTextBuilder.hpp>
#include <Nazara/TextRenderer/RichTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>
#include <Nazara/Widgets/ImageButtonWidget.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace tsom
{
	constexpr std::array s_availableBlocks = { "empty", "hull", "hull2" };

	ShipEditionState::ShipEditionState(std::shared_ptr<StateData> stateDataPtr) :
	WidgetState(std::move(stateDataPtr)),
	m_cameraMovement(false)
	{
		StateData& stateData = GetStateData();
		auto& filesystem = stateData.app->GetComponent<Nz::FilesystemAppComponent>();

		m_cameraEntity = CreateEntity();
		{
			auto& cameraNode = m_cameraEntity.emplace<Nz::NodeComponent>();
			cameraNode.SetPosition(Nz::Vector3f::Up() * 16.f);

			auto passList = filesystem.Load<Nz::PipelinePassList>("assets/3d.passlist");

			auto& cameraComponent = m_cameraEntity.emplace<Nz::CameraComponent>(stateData.renderTarget, std::move(passList));
			cameraComponent.UpdateClearColor(Nz::Color::Gray());
			cameraComponent.UpdateRenderMask(0x0000FFFF);
			cameraComponent.UpdateZNear(0.1f);
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

			// Load skybox
			Nz::Image skyboxImage(Nz::ImageType::Cubemap, Nz::PixelFormat::RGBA8_SRGB, 2048, 2048);
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveX, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/leftImage.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeX, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/rightImage.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveY, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/upImage.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeY, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/downImage.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveZ, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/frontImage.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeZ, *filesystem.Load<Nz::Image>("assets/DeepSpaceGreenSkybox/backImage.png"));

			std::shared_ptr<Nz::Texture> skyboxTexture = Nz::Texture::CreateFromImage(skyboxImage, *filesystem.GetDefaultResourceParameters<Nz::Texture>());

			// Instantiate the material to use it, and configure it (texture + cull front faces as the render is from the inside)
			std::shared_ptr<Nz::MaterialInstance> skyboxMat = skyboxMaterial->Instantiate();
			skyboxMat->SetTextureProperty("BaseColorMap", skyboxTexture);
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

		m_sunLightEntity = CreateEntity();
		{
			m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(0x0000FFFF);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(2048);
		}

		m_ship = std::make_unique<Ship>(*stateData.blockLibrary, Nz::Vector3ui(20, 20, 20), 2.f);

		m_infoLabel = CreateWidget<Nz::LabelWidget>();

		m_blockSelectionWidget = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);
		m_blockSelectionWidget->EnableBackground(true);
		m_blockSelectionWidget->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.8f));

		for (std::string_view blockName : s_availableBlocks)
		{
			BlockIndex blockIndex = stateData.blockLibrary->GetBlockIndex(blockName);
			if (blockIndex == InvalidBlockIndex)
				continue;

			std::shared_ptr<Nz::MaterialInstance> inventoryMaterial = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			inventoryMaterial->SetTextureProperty("BaseColorMap", stateData.blockLibrary->GetPreviewTexture(blockIndex));

			Nz::ImageButtonWidget* button = m_blockSelectionWidget->Add<Nz::ImageButtonWidget>(inventoryMaterial);

			button->OnButtonTrigger.Connect([this, blockIndex](const Nz::ImageButtonWidget*)
			{
				m_currentBlock = blockIndex;
			});
		}
	}

	ShipEditionState::~ShipEditionState()
	{
	}

	void ShipEditionState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		StateData& stateData = GetStateData();
		m_shipEntities = std::make_unique<ClientChunkEntities>(*stateData.app, *stateData.world, *m_ship, *stateData.blockLibrary);

		Nz::WindowEventHandler& eventHandler = GetStateData().window->GetEventHandler();
		ConnectSignal(stateData.canvas->OnUnhandledMouseMoved, [&, camAngles = Nz::EulerAnglesf(0.f, 0.f, 0.f)](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseMoveEvent& event) mutable
		{
			if (!m_cameraMovement)
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

		ConnectSignal(stateData.canvas->OnUnhandledMouseButtonPressed, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (event.button != Nz::Mouse::Right)
				return;

			m_cameraMovement = true;

			Nz::Mouse::SetRelativeMouseMode(true);
		});

		ConnectSignal(stateData.canvas->OnUnhandledMouseButtonReleased, [&](const Nz::WindowEventHandler*, const Nz::WindowEvent::MouseButtonEvent& event)
		{
			if (event.button != Nz::Mouse::Left && event.button != Nz::Mouse::Right)
				return;

			if (event.button == Nz::Mouse::Right)
			{
				m_cameraMovement = false;
				Nz::Mouse::SetRelativeMouseMode(false);
			}

			Nz::Vector3f hitPos, hitNormal;
			auto filter = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
			{
				//if (hitInfo.hitEntity != m_planetEntity)
				//  return std::nullopt;

				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				return hitInfo.fraction;
			};

			auto& camera = m_cameraEntity.get<Nz::CameraComponent>();

			Nz::Vector2f mousePos = Nz::Vector2f(event.x, event.y);

			Nz::Vector3f startPos = camera.Unproject({ mousePos.x, mousePos.y, 0.f });
			Nz::Vector3f endPos = camera.Unproject({ mousePos.x, mousePos.y, 1.f });

			auto& physSystem = stateData.world->GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQuery(startPos, endPos, filter))
			{
				if (event.button == Nz::Mouse::Left)
				{
					float sign = (m_currentBlock != EmptyBlockIndex) ? 1.f : -1.f;

					auto coordinates = m_ship->GetChunk().ComputeCoordinates(hitPos + sign * hitNormal * m_ship->GetTileSize() * 0.25f);
					if (!coordinates)
						return;

					std::lock_guard lock(m_integrityMutex);

					m_ship->GetChunk().UpdateBlock(*coordinates, m_currentBlock);
					CheckHullIntegrity();
				}
			}
		});

		CheckHullIntegrity();
	}

	void ShipEditionState::Leave(Nz::StateMachine& fsm)
	{
		if (m_integrityThread.joinable())
			m_integrityThread.join();

		WidgetState::Leave(fsm);

		Nz::Mouse::SetRelativeMouseMode(false);

		m_shipEntities.reset();
	}

	bool ShipEditionState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		constexpr unsigned int ChunkBlockCount = Ship::ChunkSize * Ship::ChunkSize * Ship::ChunkSize;

		WidgetState::Update(fsm, elapsedTime);

		m_shipEntities->Update();

		#if 0
		auto& debugDrawer = GetStateData().world->GetSystem<Nz::RenderSystem>().GetFramePipeline().GetDebugDrawer();
		for (const Area& area : m_shipAreas)
		{
			for (std::size_t blockIndex : area.blocks.IterBits())
			{
				std::size_t localBlockIndex = blockIndex % ChunkBlockCount;
				std::size_t chunkIndex = blockIndex / ChunkBlockCount;
				Chunk* chunk = m_ship->GetChunk(chunkIndex);
				if (!chunk)
					continue;

				Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> blockCorners = chunk->ComputeVoxelCorners(m_ship->GetBlockIndices({0, 0, 0}, localBlockIndex));

				debugDrawer.DrawBoxCorners(blockCorners, Nz::Color::Blue());
			}
		}
		#endif

		float cameraSpeed = (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::LShift)) ? 50.f : 10.f;
		float updateTime = elapsedTime.AsSeconds();

		auto& cameraNode = m_cameraEntity.get<Nz::NodeComponent>();
		if (m_cameraMovement)
		{
			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Space))
				cameraNode.MoveGlobal(Nz::Vector3f::Up() * cameraSpeed * updateTime);

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Z))
				cameraNode.Move(Nz::Vector3f::Forward() * cameraSpeed * updateTime);

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::S))
				cameraNode.Move(Nz::Vector3f::Backward() * cameraSpeed * updateTime);

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::Q))
				cameraNode.Move(Nz::Vector3f::Left() * cameraSpeed * updateTime);

			if (Nz::Keyboard::IsKeyPressed(Nz::Keyboard::VKey::D))
				cameraNode.Move(Nz::Vector3f::Right() * cameraSpeed * updateTime);
		}

		DrawHoveredFace();

		return true;
	}

	auto ShipEditionState::BuildArea(std::size_t firstBlockIndex, Nz::Bitset<Nz::UInt64>& remainingBlocks) const -> Area
	{
#if 0
		constexpr unsigned int ChunkBlockCount = Ship::ChunkSize * Ship::ChunkSize * Ship::ChunkSize;

		std::size_t chunkCount = m_ship->GetChunkCount();

		Nz::Bitset<Nz::UInt64> areaBlocks;

		std::vector<std::size_t> candidateBlocks;
		candidateBlocks.push_back(firstBlockIndex);

		while (!candidateBlocks.empty())
		{
			std::size_t blockIndex = candidateBlocks.back();
			candidateBlocks.pop_back();

			areaBlocks.UnboundedSet(blockIndex);

			std::size_t localBlockIndex = blockIndex % ChunkBlockCount;
			std::size_t chunkIndex = blockIndex / ChunkBlockCount;
			Chunk* chunk = m_ship->GetChunk(chunkIndex);
			if (!chunk)
				continue;

			remainingBlocks[blockIndex] = false;

			Nz::Vector3ui chunkSize = chunk->GetSize();

			BlockIndex block = chunk->GetBlockContent(localBlockIndex);
			bool isEmpty = block == EmptyBlockIndex;

			auto AddCandidateBlock = [&](const Nz::Vector3ui& blockIndices)
			{
				std::size_t blockIndex = chunk->GetBlockIndex(blockIndices);
				if (remainingBlocks[blockIndex])
				{
					if (!isEmpty)
					{
						// Non-empty blocks can look at other non-empty blocks
						if (chunk->GetBlockContent(blockIndex) != EmptyBlockIndex)
							candidateBlocks.push_back(blockIndex);
					}
					else
						candidateBlocks.push_back(blockIndex);
				}
			};

			Nz::Vector3ui blockIndices = chunk->GetBlockIndices(blockIndex);
			for (int zOffset = -1; zOffset <= 1; ++zOffset)
			{
				for (int yOffset = -1; yOffset <= 1; ++yOffset)
				{
					for (int xOffset = -1; xOffset <= 1; ++xOffset)
					{
						if (xOffset == 0 && yOffset == 0 && zOffset == 0)
							continue;

						Nz::Vector3i candidateIndicesSigned = Nz::Vector3i(blockIndices);
						candidateIndicesSigned.x += xOffset;
						candidateIndicesSigned.y += yOffset;
						candidateIndicesSigned.z += zOffset;

						Nz::Vector3ui candidateIndices = Nz::Vector3ui(candidateIndicesSigned);
						if (candidateIndices.x < chunkSize.x && candidateIndices.y < chunkSize.y && candidateIndices.z < chunkSize.z)
							AddCandidateBlock(candidateIndices);
					}
				}
			}
		}
		#endif
		Area area;
		//area.blocks = std::move(areaBlocks);

		return area;
	}

	void ShipEditionState::CheckHullIntegrity()
	{
		if (m_integrityThread.joinable())
			m_integrityThread.join();

		m_integrityThread = std::thread([this]
		{
			constexpr unsigned int ChunkBlockCount = Ship::ChunkSize * Ship::ChunkSize * Ship::ChunkSize;

			std::lock_guard lock(m_integrityMutex);

			Nz::HighPrecisionClock clock;

			std::size_t chunkCount = m_ship->GetChunkCount();

			Nz::Bitset<Nz::UInt64> remainingBlocks(chunkCount * ChunkBlockCount, true);

			// Find first candidate (= a random empty block)
			auto FindFirstCandidate = [&]
			{
				for (std::size_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
				{
					#if 0
					const Chunk* chunk = m_ship->GetChunk(chunkIndex);
					if (!chunk)
						continue; //< TODO: Empty chunks should be handled in visitedBlock/candidateBlock (?)

					const Nz::Bitset<Nz::UInt64>& collisionCellMask = chunk->GetCollisionCellMask();
					for (std::size_t i = 0; i < collisionCellMask.GetBlockCount(); ++i)
					{
						Nz::UInt64 mask = collisionCellMask.GetBlock(i);
						mask = ~mask;

						unsigned int fsb = Nz::FindFirstBit(mask);
						if (fsb != 0)
						{
							unsigned int localBlockIndex = i * Nz::BitCount<Nz::UInt64>() + fsb - 1;
							return chunkIndex * ChunkBlockCount + localBlockIndex;
						}
					}
					#endif
				}

				return std::numeric_limits<std::size_t>::max();
			};

			m_shipAreas.clear();
			std::size_t firstCandidate = FindFirstCandidate();
			if (firstCandidate != std::numeric_limits<std::size_t>::max())
			{
				Area outside = BuildArea(FindFirstCandidate(), remainingBlocks);

				while (remainingBlocks.TestAny())
					m_shipAreas.push_back(BuildArea(remainingBlocks.FindFirst(), remainingBlocks));
			}

			fmt::print("integrity check took {}\n", fmt::streamed(clock.GetElapsedTime()));

			Nz::RichTextDrawer richText;
			Nz::RichTextBuilder builder(richText);
			builder << Nz::Color::Yellow() << "Hull integrity: ";

			if (m_shipAreas.empty())
				builder << Nz::Color::Red() << "KO";
			else
				builder << Nz::Color::Green() << "OK";

			builder << Nz::Color::White() << " (" << std::to_string(m_shipAreas.size()) << " area(s))";

			UpdateStatus(richText);
		});
	}

	void ShipEditionState::DrawHoveredFace()
	{
		auto& camera = m_cameraEntity.get<Nz::CameraComponent>();

		Nz::Vector2f mousePos = Nz::Vector2f(Nz::Mouse::GetPosition(*GetStateData().window));

		Nz::Vector3f startPos = camera.Unproject({ mousePos.x, mousePos.y, 0.f });
		Nz::Vector3f endPos = camera.Unproject({ mousePos.x, mousePos.y, 1.f });
#if 0

		// Raycast
		auto& physSystem = GetStateData().world->GetSystem<Nz::Physics3DSystem>();
		Nz::Vector3f hitPos, hitNormal;
		if (physSystem.RaycastQuery(startPos, endPos, [&](const Nz::Physics3DSystem::RaycastHit& hitInfo) -> std::optional<float>
		{
			//if (hitInfo.hitEntity != m_planetEntity)
			//  return std::nullopt;

			hitPos = hitInfo.hitPosition;
			hitNormal = hitInfo.hitNormal;
			return hitInfo.fraction;
		}))
		{
			Nz::Vector3f localPos;
			if (const Chunk* chunk = m_ship->GetChunkByPosition(hitPos - hitNormal * m_ship->GetTileSize() * 0.25f, &localPos))
			{
				auto coordinates = chunk->ComputeCoordinates(localPos);
				if (coordinates)
				{
					auto cornerPos = chunk->ComputeVoxelCorners(*coordinates);
					Nz::Vector3f offset = m_ship->GetChunkOffset(chunk->GetIndices());

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

					auto& debugDrawer = GetStateData().world->GetSystem<Nz::RenderSystem>().GetFramePipeline().GetDebugDrawer();
					debugDrawer.DrawLine(offset + cornerPos[corners[0]], offset + cornerPos[corners[1]], Nz::Color::Green());
					debugDrawer.DrawLine(offset + cornerPos[corners[1]], offset + cornerPos[corners[2]], Nz::Color::Green());
					debugDrawer.DrawLine(offset + cornerPos[corners[2]], offset + cornerPos[corners[3]], Nz::Color::Green());
					debugDrawer.DrawLine(offset + cornerPos[corners[3]], offset + cornerPos[corners[0]], Nz::Color::Green());
				}
			}
		}
#endif
	}

	void ShipEditionState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_blockSelectionWidget->Resize(Nz::Vector2f(s_availableBlocks.size() * 128.f, 128.f));
		m_blockSelectionWidget->SetPosition({ 0.f, newSize.y * 0.5f - m_blockSelectionWidget->GetHeight() * 0.5f, 0.f });

		m_infoLabel->SetPosition(newSize * Nz::Vector2f(0.5f, 0.98f) - m_infoLabel->GetSize() * 0.5f);
	}

	void ShipEditionState::UpdateStatus(const Nz::AbstractTextDrawer& textDrawer)
	{
		m_infoLabel->UpdateText(textDrawer);
		m_infoLabel->Center();
		m_infoLabel->Show();

		LayoutWidgets(GetStateData().canvas->GetSize());
	}
}
