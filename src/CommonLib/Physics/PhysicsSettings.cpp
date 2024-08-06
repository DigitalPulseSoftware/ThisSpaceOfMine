// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Physics/ContactCallbackComponents.hpp>

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
						return Constants::BroadphaseStatic;

					case Constants::ObjectLayerDynamic:
					case Constants::ObjectLayerDynamicNoPlayer:
					case Constants::ObjectLayerDynamicTrigger:
					case Constants::ObjectLayerPlayer:
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

			void OnContactAdded(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2) override
			{
				if (ContactAddedCallbackComponent* callbackComponent = entity1.try_get<ContactAddedCallbackComponent>())
					callbackComponent->callback(entity1, body1, entity2, body2);

				if (ContactAddedCallbackComponent* callbackComponent = entity2.try_get<ContactAddedCallbackComponent>())
					callbackComponent->callback(entity2, body2, entity1, body1);
			}

			void OnContactPersisted(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2) override
			{
			}

			void OnContactRemoved(entt::handle entity1, const Nz::PhysBody3D* body1, entt::handle entity2, const Nz::PhysBody3D* body2) override
			{
				if (ContactRemovedCallbackComponent* callbackComponent = entity1.try_get<ContactRemovedCallbackComponent>())
					callbackComponent->callback(entity1, body1, entity2, body2);

				if (ContactRemovedCallbackComponent* callbackComponent = entity2.try_get<ContactRemovedCallbackComponent>())
					callbackComponent->callback(entity2, body2, entity1, body1);
			}
		};
	}

	std::unique_ptr<Nz::Physics3DSystem::ContactListener> tsom::Physics::BuildContactListener()
	{
		return std::make_unique<ContactListenerBridge>();
	}

	Nz::Physics3DSystem::Settings tsom::Physics::BuildSettings()
	{
		Nz::Physics3DSystem::Settings physSettings = Nz::PhysWorld3D::BuildDefaultSettings();
		physSettings.broadphaseLayerInterface = std::make_unique<BroadphaseLayerInterface>();
		physSettings.objectLayerPairFilter = std::make_unique<PhysObjectLayerPairFilter3D>();
		physSettings.objectVsBroadphaseLayerFilter = std::make_unique<PhysObjectVsBroadphaseLayerFilter3D>();

		return physSettings;
	}
}
