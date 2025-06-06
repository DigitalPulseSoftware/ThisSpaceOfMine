// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientEnvironmentHandler.hpp>
#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <Nazara/Core/Animation.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Primitive.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialSettings.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PredefinedMaterials.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Graphics/PropertyHandler/UniformValuePropertyHandler.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	void ClientEnvironmentHandler::SetupRootEntities()
	{
		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();

		m_sunLightEntity = m_world.CreateEntity();
		{
			auto& lightNode = m_sunLightEntity.emplace<Nz::NodeComponent>(Nz::Vector3f::Zero(), Nz::EulerAnglesf(-30.f, 80.f, 0.f));
			lightNode.SetParent(m_rootTransform);

			auto& lightComponent = m_sunLightEntity.emplace<Nz::LightComponent>();
			auto& dirLight = lightComponent.AddLight<Nz::DirectionalLight>(tsom::Constants::RenderMask3D);
			dirLight.UpdateAmbientFactor(0.05f);
			dirLight.EnableShadowCasting(true);
			dirLight.UpdateShadowMapSize(2048);
			dirLight.UpdateEnergy(5.f);
			dirLight.EnableFixedShadowCascadSplit(true);
			dirLight.UpdateShadowCascadeFixedSplitFactors(m_csmSplitFactors);

			m_directionalLight = &dirLight;
		}

		m_skyboxEntity = m_world.CreateEntity();
		{
			// Create a new material (custom properties + shaders) for the skybox
			Nz::MaterialSettings skyboxSettings;
			skyboxSettings.AddValueProperty<Nz::Color>("BaseColor", Nz::Color::White());
			skyboxSettings.AddValueProperty<float>("Rotation", 0.f);
			skyboxSettings.AddValueProperty<Nz::Vector3f>("RotationAxis", Nz::Vector3f::UnitY());
			skyboxSettings.AddTextureProperty("BaseColorMap", Nz::ImageType::Cubemap);
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("BaseColorMap", "HasBaseColorTexture"));
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("BaseColor"));
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("Rotation"));
			skyboxSettings.AddPropertyHandler(std::make_unique<Nz::UniformValuePropertyHandler>("RotationAxis"));

			// Setup only a forward pass (using the SkyboxMaterial module)
			Nz::MaterialPass forwardPass;
			forwardPass.states.depthBuffer = true;
			forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
			forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "SkyboxMaterial"));
			skyboxSettings.AddPass("ForwardPass", forwardPass);

			// Finalize the material (using SkyboxMaterial module as a reference for shader reflection)
			std::shared_ptr<Nz::Material> skyboxMaterial = std::make_shared<Nz::Material>(std::move(skyboxSettings), "SkyboxMaterial");

			// Instantiate the material to use it, and configure it (texture + cull front faces as the render is from the inside)
			m_skyboxMaterial = skyboxMaterial->Instantiate();
			m_skyboxMaterial->SetTextureProperty("BaseColorMap", fs.Open<Nz::TextureAsset>("assets/skybox-space.png", { .sRGB = true }, Nz::CubemapParams{}));
			m_skyboxMaterial->UpdatePassesStates([](Nz::RenderStates& states)
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
			skyboxModel->SetMaterial(0, m_skyboxMaterial);

			// Attach the model to the entity
			m_skyboxEntity.emplace<Nz::GraphicsComponent>(std::move(skyboxModel), tsom::Constants::RenderMask3D);

			// Setup entity position and attach it to the camera (position only, camera rotation does not impact skybox) so it's not culled
			auto& skyboxNode = m_skyboxEntity.emplace<Nz::NodeComponent>();
			skyboxNode.SetInheritRotation(false);
			skyboxNode.SetInheritScale(false);
			skyboxNode.SetParent(m_cameraEntity);
		}
	}

	void ClientEnvironmentHandler::SetupPlayerModel()
	{
		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();

		m_playerCollider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);

		m_playerAnimAssets = std::make_shared<PlayerAnimationAssets>();

		Nz::ModelParams params;
		params.loadMaterials = false;
		params.mesh.vertexDeclaration = Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Normal_UV_Tangent_Skinning);
		params.meshCallback = [&](const std::shared_ptr<Nz::Mesh>& mesh) -> Nz::Result<void, Nz::ResourceLoadingError>
		{
			if (!mesh->IsAnimable())
				return Nz::Err(Nz::ResourceLoadingError::Unrecognized);

			m_playerAnimAssets->referenceSkeleton = std::move(*mesh->GetSkeleton());
			return Nz::Ok();
		};

		params.mesh.vertexOffset = Nz::Vector3f(0.f, -0.826f, 0.f);
		params.mesh.vertexRotation = Nz::Quaternionf(Nz::TurnAnglef(0.5f), Nz::Vector3f::Up());
		params.mesh.vertexScale = Nz::Vector3f(1.f / 10.f);

		m_playerModel = fs.Load<Nz::Model>("assets/Player/Idle.fbx", params);
		if (m_playerModel)
		{
			assert(m_playerAnimAssets->referenceSkeleton.IsValid());

			Nz::AnimationParams animParams;
			animParams.skeleton = &m_playerAnimAssets->referenceSkeleton;

			animParams.jointOffset = params.mesh.vertexOffset;
			animParams.jointRotation = params.mesh.vertexRotation;
			animParams.jointScale = params.mesh.vertexScale;

			Nz::MaterialSettings settings;
			Nz::PredefinedMaterials::AddBasicSettings(settings);
			Nz::PredefinedMaterials::AddPbrSettings(settings);
			settings.AddTextureProperty("AmbientOcclusionMap", Nz::ImageType::E2D);
			settings.AddTextureProperty("MetalnessSmoothnessMap", Nz::ImageType::E2D);
			settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("AmbientOcclusionMap", "HasAmbientOcclusionTexture"));
			settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("MetalnessSmoothnessMap", "HasMetalnessSmoothnessTexture"));

			Nz::MaterialPass forwardPass;
			forwardPass.states.depthBuffer = true;
			forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
			forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.PlayerPBR"));
			settings.AddPass("ForwardPass", forwardPass);

			Nz::MaterialPass depthPass = forwardPass;
			depthPass.options[nzsl::Ast::HashOption("DepthPass")] = true;
			settings.AddPass("DepthPass", depthPass);

			Nz::MaterialPass shadowPass = depthPass;
			shadowPass.options[nzsl::Ast::HashOption("ShadowPass")] = true;
			shadowPass.states.depthCompare = Nz::RendererComparison::LessOrEqual; //< TODO: Reverse depth for shadow pass?
			shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
			shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetRenderDevice()->GetEnabledFeatures().depthClamping;
			settings.AddPass("ShadowPass", shadowPass);

			Nz::MaterialPass distanceShadowPass = shadowPass;
			distanceShadowPass.options[nzsl::Ast::HashOption("DistanceDepth")] = true;
			settings.AddPass("DistanceShadowPass", distanceShadowPass);

			auto playerMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.PlayerPBR");

			std::shared_ptr<Nz::MaterialInstance> playerMat = playerMaterial->Instantiate();
			playerMat->SetTextureProperty("BaseColorMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_AlbedoTransparency.png", { .sRGB = true }));
			playerMat->SetTextureProperty("AmbientOcclusionMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_AO.png"));
			playerMat->SetTextureProperty("MetalnessSmoothnessMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_Normal.png"));
			playerMat->SetTextureProperty("NormalMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_Normal.png"));

			m_playerModel->SetMaterial(0, std::move(playerMat));

			m_playerAnimAssets->idleAnimation = fs.Load<Nz::Animation>("assets/Player/Idle.fbx", animParams);
			m_playerAnimAssets->runningAnimation = fs.Load<Nz::Animation>("assets/Player/Running.fbx", animParams);
			m_playerAnimAssets->walkingAnimation = fs.Load<Nz::Animation>("assets/Player/Walking.fbx", animParams);
		}
		else
		{
			// Fallback
			std::shared_ptr<Nz::Mesh> mesh = Nz::Mesh::Build(m_playerCollider->GenerateDebugMesh());

			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*mesh);

			m_playerModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < m_playerModel->GetSubMeshCount(); ++i)
				m_playerModel->SetMaterial(i, colliderMat);
		}
	}
}
