// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/BackgroundState.hpp>
#include <Nazara/Core/AppFilesystemComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Primitive.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Graphics.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Utility.hpp>
#include <Nazara/Widgets.hpp>
#include <random>

namespace tsom
{
	BackgroundState::BackgroundState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData)
	{
		auto& filesystem = stateData->app->GetComponent<Nz::AppFilesystemComponent>();

		m_camera = CreateEntity();
		{
			std::random_device rd;
			std::uniform_real_distribution<float> dis(-180.f, 180.f);

			auto& cameraNode = m_camera.emplace<Nz::NodeComponent>();
			cameraNode.SetRotation(Nz::EulerAnglesf(dis(rd), dis(rd), dis(rd)));

			auto& cameraComponent = m_camera.emplace<Nz::CameraComponent>(GetStateData().swapchain, Nz::ProjectionType::Perspective);
			cameraComponent.UpdateFOV(Nz::DegreeAnglef(45.f));
			cameraComponent.UpdateRenderMask(0x0000FFFF);
			cameraComponent.UpdateRenderOrder(-1);
		}

		m_skybox = CreateEntity();
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
			Nz::Image skyboxImage(Nz::ImageType::Cubemap, Nz::PixelFormat::RGBA8, 2048, 2048);
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveX, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_right1.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeX, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_left2.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveY, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_top3.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeY, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_bottom4.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::PositiveZ, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_front5.png"));
			skyboxImage.LoadFaceFromImage(Nz::CubemapFace::NegativeZ, *filesystem.Load<Nz::Image>("assets/PurpleNebulaSkybox/purple_nebula_skybox_back6.png"));

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
			m_skybox.emplace<Nz::GraphicsComponent>(std::move(skyboxModel));

			// Setup entity position and attach it to the camera (position only, camera rotation does not impact skybox)
			auto& skyboxNode = m_skybox.emplace<Nz::NodeComponent>();
			skyboxNode.SetInheritRotation(false);
		}
	}

	bool BackgroundState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		float dt = elapsedTime.AsSeconds();
		m_camera.get<Nz::NodeComponent>().Rotate(Nz::EulerAnglesf(dt, dt * 1.5f, 0.f));

		return true;
	}
}
