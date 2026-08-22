// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/GravityController.hpp>
#include <CommonLib/Components/BuoyancyComponent.hpp>
#include <CommonLib/Debug/DebugDrawInterface.hpp>
#include <Nazara/Core/Color.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Physics3D/PhysContact3D.hpp>
#include <Nazara/Physics3D/PhysWorld3D.hpp>
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	BuoyancySystem::BuoyancySystem(entt::registry& registry, const GravityController& gravityController, Nz::PhysWorld3D& physWorld, DebugDrawInterface* debugDrawInterface) :
	m_registry(registry),
	m_physWorld(physWorld),
	m_debugDrawInterface(debugDrawInterface),
	m_gravityController(gravityController)
	{
		m_physWorld.RegisterStepListener(this);
	}

	BuoyancySystem::~BuoyancySystem()
	{
		m_physWorld.UnregisterStepListener(this);
	}

	void BuoyancySystem::HandleContactAdded(entt::handle /*entity*/, const Nz::PhysBody3D* body, entt::handle otherEntity, const Nz::PhysBody3D* /*otherBody*/, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& /*physContactResponse*/)
	{
		BuoyancyComponent* buoyancyComponent = otherEntity.try_get<BuoyancyComponent>();
		if (!buoyancyComponent)
			return;

		std::lock_guard lock(buoyancyComponent->mutex);
		if (buoyancyComponent->contactCount.load(std::memory_order::relaxed) >= buoyancyComponent->contacts.size())
			return;

		unsigned int contactIndex = buoyancyComponent->contactCount++;
		auto& contactData = buoyancyComponent->contacts[contactIndex];
		contactData.fluidBodyIndex = body->GetBodyIndex();
		contactData.fluidSubshapeID = physContact.subshapeID1;
		contactData.bodySubshapeID = physContact.subshapeID2;

		contactData.contactIndex = 0;
		contactData.contactPos = physContact.baseOffset + physContact.relativeContactPositions1[contactData.contactIndex];
	};

	void BuoyancySystem::HandleContactPersisted(entt::handle /*entity*/, const Nz::PhysBody3D* body, entt::handle otherEntity, const Nz::PhysBody3D* /*otherBody*/, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& /*physContactResponse*/)
	{
		BuoyancyComponent* buoyancyComponent = otherEntity.try_get<BuoyancyComponent>();
		if (!buoyancyComponent)
			return;

		std::lock_guard lock(buoyancyComponent->mutex);
		unsigned int contactCount = buoyancyComponent->contactCount.load(std::memory_order::relaxed);

		auto contactBegin = buoyancyComponent->contacts.begin();
		auto contactEnd = buoyancyComponent->contacts.begin() + contactCount;

		Nz::UInt32 fluidBodyIndex = body->GetBodyIndex();
		Nz::UInt32 subShapeID1 = physContact.subshapeID1;
		Nz::UInt32 subShapeID2 = physContact.subshapeID2;

		auto it = std::find_if(
			contactBegin,
			contactEnd,
			[&](const BuoyancyComponent::FluidContact& contact) { return contact.fluidBodyIndex == fluidBodyIndex && contact.fluidSubshapeID == subShapeID1 && contact.bodySubshapeID == subShapeID2; });

		if (it == contactEnd)
			return;

		auto& contactData = *it;
		// Cycle contact points
		if (++contactData.contactIndex >= physContact.relativeContactPositions1.size())
			contactData.contactIndex = 0;

		contactData.contactPos = physContact.baseOffset + physContact.relativeContactPositions1[it->contactIndex];
	};

	void BuoyancySystem::HandleContactRemoved(entt::handle /*entity1*/, Nz::UInt32 body1Index, const Nz::PhysBody3D* /*body1*/, Nz::UInt32 subShapeID1, entt::handle entity2, Nz::UInt32 /*body2Index*/, const Nz::PhysBody3D* /*body2*/, Nz::UInt32 subShapeID2)
	{
		BuoyancyComponent* buoyancyComponent = entity2.try_get<BuoyancyComponent>();
		if (!buoyancyComponent)
			return;

		std::lock_guard lock(buoyancyComponent->mutex);
		unsigned int contactCount = buoyancyComponent->contactCount.load(std::memory_order::relaxed);

		auto contactBegin = buoyancyComponent->contacts.begin();
		auto contactEnd = buoyancyComponent->contacts.begin() + contactCount;

		auto it = std::find_if(
			contactBegin,
			contactEnd,
			[&](const BuoyancyComponent::FluidContact& contact) { return contact.fluidBodyIndex == body1Index && contact.fluidSubshapeID == subShapeID1 && contact.bodySubshapeID == subShapeID2; });

		if (it == contactEnd)
			return;

		if (contactCount > 1 && it != contactEnd - 1)
			*it = std::move(*(contactEnd - 1));

		buoyancyComponent->contactCount.fetch_sub(1, std::memory_order::relaxed);
	};

	void BuoyancySystem::PreSimulate(float elapsedTime)
	{
		Nz::HybridVector<Nz::Vector3f, 32> debugLines;

		// FIXME: For now player water detection is performed in ServerCharacterController

		/*auto playerView = m_registry.view<Nz::PhysCharacter3DComponent, BuoyancyComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, character, buoyancy] : playerView.each())
		{
			CharacterController& controller = Nz::SafeCast<CharacterController&>(*character.GetImpl());

			unsigned int contactCount = buoyancy.contactCount.load(std::memory_order::relaxed);
			if (contactCount == 0)
			{
				controller.SetInWater(false);
				continue;
			}

			controller.SetInWater(true);
			const auto& fluidContact = buoyancy.contacts.front();

			auto gravity = m_gravityController.ComputeGravity(fluidContact.contactPos);

			bool isInWater = character.ApplyBuoyancyImpulse(fluidContact.contactPos, -gravity.direction, 1.f, 0.75f, 0.01f, Nz::Vector3f::Zero(), gravity.acceleration * gravity.direction * gravity.factor, elapsedTime);
			if (isInWater && m_debugDrawInterface)
			{
				debugLines.push_back(fluidContact.contactPos);
				debugLines.push_back(fluidContact.contactPos - gravity.direction);
			}
		}*/

		auto view = m_registry.view<Nz::RigidBody3DComponent, BuoyancyComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, rigidBody, buoyancy] : view.each())
		{
			unsigned int contactCount = buoyancy.contactCount.load(std::memory_order::relaxed);
			if (contactCount == 0 || rigidBody.IsSleeping())
				continue;

			float invContactCount = 1.f / contactCount;
			for (unsigned int i = 0; i < contactCount; ++i)
			{
				const auto& fluidContact = buoyancy.contacts[i];

				auto gravity = m_gravityController.ComputeGravity(fluidContact.contactPos);

				bool isInWater = rigidBody.ApplyBuoyancyImpulse(fluidContact.contactPos, -gravity.direction, 1.5f * invContactCount, 0.75f, 0.01f, Nz::Vector3f::Zero(), gravity.acceleration * gravity.direction * gravity.factor, elapsedTime);
				if (isInWater && m_debugDrawInterface)
				{
					debugLines.push_back(fluidContact.contactPos);
					debugLines.push_back(fluidContact.contactPos - gravity.direction);
				}
			}
		}

		if (!debugLines.empty())
		{
			NazaraAssert(m_debugDrawInterface);
			m_debugDrawInterface->DrawLines(std::hash<std::string_view>{}("Buoyancy"), 0.1f, debugLines, Nz::Color::Cyan());
		}
	}
}
