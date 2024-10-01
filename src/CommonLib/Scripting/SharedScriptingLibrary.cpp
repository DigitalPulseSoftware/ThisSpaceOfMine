// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/SharedScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ShipController.hpp>

namespace tsom
{
	void SharedScriptingLibrary::Register(sol::state& state)
	{
		RegisterMovementControllers(state);
	}

	void SharedScriptingLibrary::RegisterMovementControllers(sol::state& state)
	{
		state.new_usertype<CharacterController>("CharacterController",
			sol::no_constructor,
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
}
