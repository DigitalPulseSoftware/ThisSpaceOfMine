// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/PhysicsConstants.hpp>

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
