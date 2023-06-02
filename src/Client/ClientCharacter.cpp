// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Client/ClientCharacter.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/JoltPhysics3D/JoltCharacter.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltCharacterComponent.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <Client/CharacterController.hpp>

namespace tsom
{
	ClientCharacter::ClientCharacter(Nz::EnttWorld& world, const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		auto& physicsSystem = world.GetSystem<Nz::JoltPhysics3DSystem>();

		std::shared_ptr<Nz::JoltCollider3D> collider = BuildCollider();

		m_entity = world.CreateEntity();
		m_entity.emplace<Nz::NodeComponent>(position, rotation);
		
		m_controller = std::make_shared<CharacterController>();

		auto& characterComponent = m_entity.emplace<Nz::JoltCharacterComponent>(physicsSystem.CreateCharacter(collider, position, rotation));
		characterComponent.SetImpl(m_controller);
		characterComponent.DisableSleeping();

		std::shared_ptr<Nz::Model> colliderModel;
		{
			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::Graphics::Instance()->GetDefaultMaterials().basicMaterial->Instantiate();
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(collider->GenerateDebugMesh());
			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

			colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
				colliderModel->SetMaterial(i, colliderMat);

			auto& gfx = m_entity.emplace<Nz::GraphicsComponent>();
			gfx.AttachRenderable(std::move(colliderModel), 0x0000FFFF);
		}
	}

	void ClientCharacter::DebugDraw(Nz::DebugDrawer& debugDrawer)
	{
		auto& characterComponent = m_entity.get<Nz::JoltCharacterComponent>();
		m_controller->DebugDraw(characterComponent, debugDrawer);
	}

	void ClientCharacter::SetCurrentPlanet(const Planet* planet)
	{
		m_controller->SetCurrentPlanet(planet);
	}

	std::shared_ptr<Nz::JoltCollider3D> ClientCharacter::BuildCollider()
	{
		return std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);
	}
}
