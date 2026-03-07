// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/SharedScriptingLibrary.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/ShipController.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ScriptedEntityComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <CommonLib/Scripting/SharedEntityScriptingLibrary.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	void SharedScriptingLibrary::Register(sol::state& state)
	{
		RegisterMovementControllers(state);
		RegisterPhysWorld(state);
	}

	void SharedScriptingLibrary::RegisterMovementControllers(sol::state& state)
	{
		state.new_usertype<CharacterController>("CharacterController",
			sol::no_constructor,
			"GetCameraAngles", LuaFunction(&CharacterController::GetCameraAngles),
			"GetCameraRotation", LuaFunction(&CharacterController::GetCameraRotation),
			"GetCharacterPosition", LuaFunction(&CharacterController::GetCharacterPosition),
			"GetCharacterRotation", LuaFunction(&CharacterController::GetCharacterRotation),
			"GetEyePosition", LuaFunction(&CharacterController::GetEyePosition),
			"GetReferenceRotation", LuaFunction(&CharacterController::GetReferenceRotation),
			"SetShipController", LuaFunction([](CharacterController& controller, std::optional<std::shared_ptr<ShipController>> newShipController)
			{
				controller.SetShipController(std::move(newShipController).value_or(nullptr));
			})
		);

		state.new_usertype<ShipController>("ShipController",
			"new", sol::factories(LuaFunction([](sol::table shipEntity, const Nz::Quaternionf& rotation) -> std::shared_ptr<ShipController>
			{
				entt::handle entity = AssertScriptEntity(shipEntity);
				return std::make_shared<ShipController>(entity, rotation);
			}))
		);
	}

	void SharedScriptingLibrary::RegisterPhysWorld(sol::state& state)
	{
		state.new_usertype<Nz::Physics3DSystem>("PhysWorld",
			sol::no_constructor,
			"RaycastQueryFirst", LuaFunction([this](sol::this_state L, Nz::Physics3DSystem& physSystem, const Nz::Vector3f& startPos, const Nz::Vector3f& endPos, sol::optional<sol::table> parameters)
			{
				sol::state_view state(L);

				sol::object retVal = sol::nil;
				auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
				{
					ScriptedEntityComponent* scriptedEntity = nullptr;
					if (hitInfo.hitEntity)
						scriptedEntity = hitInfo.hitEntity.try_get<ScriptedEntityComponent>();

					sol::table result = state.create_table_with(
						"fraction", hitInfo.fraction,
						"hitPosition", hitInfo.hitPosition,
						"hitNormal", hitInfo.hitNormal,
						"subShapeID", hitInfo.subShapeID
					);

					if (hitInfo.hitEntity)
					{
						result["hitEntity"] = m_entityScriptingLibrary.ToEntityTable(state, hitInfo.hitEntity);

						if (auto* chunkComponent = hitInfo.hitEntity.try_get<ChunkComponent>())
							result["hitChunk"] = chunkComponent->chunk->shared_from_this();
					}

					retVal = result;
				};

				Nz::PhysObjectLayerFilter3D* objectFilter = nullptr;

				// TODO: Refactor this
				struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
				{
					bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
					{
						return layer != Constants::ObjectLayerPlayer;
					}
				};

				IgnorePlayer ignorePlayerFilter;

				if (parameters)
				{
					sol::table& parameterTable = *parameters;
					if (parameterTable["IgnorePlayers"])
						objectFilter = &ignorePlayerFilter;
				}

				physSystem.RaycastQueryFirst(startPos, endPos, callback, nullptr, objectFilter);
				return retVal;
			})
		);
	}
}
