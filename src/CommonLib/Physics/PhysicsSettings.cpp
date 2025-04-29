// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Physics/ContactCallbackComponents.hpp>
#include <Nazara/Physics3D/PhysContact3D.hpp>
#include <Nazara/Physics3D/PhysContactResponse3D.hpp>

namespace tsom::Physics
{
	namespace
	{
		struct BroadphaseLayerInterface : Nz::PhysBroadphaseLayerInterface3D
		{
			Nz::PhysBroadphase3D GetBroadphaseLayer(Nz::PhysObjectLayer3D objectLayer) const override
			{
				switch (objectLayer)
				{
					case Constants::ObjectLayerStatic:
					case Constants::ObjectLayerStaticNoPlayer:
					case Constants::ObjectLayerStaticTrigger:
					case Constants::ObjectLayerStaticWater:
						return Constants::BroadphaseStatic;

					case Constants::ObjectLayerDynamic:
					case Constants::ObjectLayerDynamicNoCollision:
					case Constants::ObjectLayerDynamicNoPlayer:
					case Constants::ObjectLayerDynamicTrigger:
					case Constants::ObjectLayerPlayer:
					case Constants::ObjectLayerPlayerOnlyTrigger:
						return Constants::BroadphaseDynamic;

					default:
						assert(!"unhandled object layer");
						return 0;
				}
			}

			unsigned int GetBroadphaseLayerCount() const override
			{
				return 2u;
			}

			const char* GetBroadphaseLayerName(Nz::PhysBroadphase3D broadphaseLayer) const override
			{
				constexpr std::array<const char*, 2> layerName = {
					"Static",
					"Dynamic"
				};

				assert(broadphaseLayer < layerName.size());
				return layerName[broadphaseLayer];
			}
		};

		struct PhysObjectLayerPairFilter3D : Nz::PhysObjectLayerPairFilter3D
		{
			bool ShouldCollide(Nz::PhysObjectLayer3D object1, Nz::PhysObjectLayer3D object2) const override
			{
				if (object1 == Constants::ObjectLayerPlayer)
				{
					if (object2 == Constants::ObjectLayerDynamicNoPlayer || object2 == Constants::ObjectLayerStaticNoPlayer)
						return false;
				}
				else if (object2 == Constants::ObjectLayerPlayer)
				{
					if (object1 == Constants::ObjectLayerDynamicNoPlayer || object1 == Constants::ObjectLayerStaticNoPlayer)
						return false;
				}
				else if (object1 == Constants::ObjectLayerDynamicNoCollision || object2 == Constants::ObjectLayerDynamicNoCollision)
					return false;
				else if (object1 == Constants::ObjectLayerPlayerOnlyTrigger)
					return object2 == Constants::ObjectLayerDynamicTrigger || object2 == Constants::ObjectLayerStaticTrigger;
				else if (object2 == Constants::ObjectLayerPlayerOnlyTrigger)
					return object1 == Constants::ObjectLayerDynamicTrigger || object1 == Constants::ObjectLayerStaticTrigger;

				return true;
			}
		};

		struct PhysObjectVsBroadphaseLayerFilter3D : Nz::PhysObjectVsBroadphaseLayerFilter3D
		{
			bool ShouldCollide(Nz::PhysObjectLayer3D objectLayer, Nz::PhysBroadphase3D broadphaseLayer) const override
			{
				return true;
			}
		};

		class ContactListenerBridge : public Nz::Physics3DSystem::ContactListener
		{
			Nz::PhysContactValidateResult3D ValidateContact(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2, const Nz::Vector3f& baseOffset, const Nz::Physics3DSystem::ShapeCollisionInfo& collisionResult) override
			{
				return Nz::PhysContactValidateResult3D::AcceptAllContactsForThisBodyPair;
			}

			void OnContactAdded(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& physContactResponse) override
			{
				if (ContactAddedCallbackComponent* callbackComponent = entity1.try_get<ContactAddedCallbackComponent>())
					callbackComponent->callback(entity1, body1, entity2, body2, physContact, physContactResponse);

				if (ContactAddedCallbackComponent* callbackComponent = entity2.try_get<ContactAddedCallbackComponent>())
				{
					physContactResponse.SwapBodies();
					callbackComponent->callback(entity2, body2, entity1, body1, physContact.SwapBodies(), physContactResponse);
					physContactResponse.SwapBodies();
				}
			}

			void OnContactPersisted(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& physContactResponse) override
			{
				if (ContactPersistedCallbackComponent* callbackComponent = entity1.try_get<ContactPersistedCallbackComponent>())
					callbackComponent->callback(entity1, body1, entity2, body2, physContact, physContactResponse);

				if (ContactPersistedCallbackComponent* callbackComponent = entity2.try_get<ContactPersistedCallbackComponent>())
				{
					physContactResponse.SwapBodies();
					callbackComponent->callback(entity2, body2, entity1, body1, physContact.SwapBodies(), physContactResponse);
					physContactResponse.SwapBodies();
				}
			}

			void OnContactRemoved(entt::handle entity1, Nz::UInt32 body1Index, const Nz::PhysBody3D* body1, Nz::UInt32 subShapeID1, entt::handle entity2, Nz::UInt32 body2Index, const Nz::PhysBody3D* body2, Nz::UInt32 subShapeID2) override
			{
				if (ContactRemovedCallbackComponent* callbackComponent = entity1.try_get<ContactRemovedCallbackComponent>())
					callbackComponent->callback(entity1, body1Index, body1, subShapeID1, entity2, body2Index, body2, subShapeID2);

				if (ContactRemovedCallbackComponent* callbackComponent = entity2.try_get<ContactRemovedCallbackComponent>())
					callbackComponent->callback(entity2, body2Index, body2, subShapeID2, entity1, body1Index, body1, subShapeID1);
			}
		};
	}

	std::unique_ptr<Nz::Physics3DSystem::ContactListener> BuildContactListener()
	{
		return std::make_unique<ContactListenerBridge>();
	}

	Nz::Physics3DSystem::Settings BuildSettings()
	{
		Nz::Physics3DSystem::Settings physSettings = Nz::PhysWorld3D::BuildDefaultSettings();
		physSettings.broadphaseLayerInterface = std::make_unique<BroadphaseLayerInterface>();
		physSettings.objectLayerPairFilter = std::make_unique<PhysObjectLayerPairFilter3D>();
		physSettings.objectVsBroadphaseLayerFilter = std::make_unique<PhysObjectVsBroadphaseLayerFilter3D>();

		return physSettings;
	}
}
