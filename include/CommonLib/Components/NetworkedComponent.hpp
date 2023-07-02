// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <Nazara/JoltPhysics3D/JoltCharacter.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>
#include <optional>

namespace tsom
{
	class NetworkedComponent
	{
		public:
			NetworkedComponent() = default;
			NetworkedComponent(const NetworkedComponent&) = delete;
			NetworkedComponent(NetworkedComponent&&) = default;
			~NetworkedComponent() = default;

			NetworkedComponent& operator=(const NetworkedComponent&) = delete;
			NetworkedComponent& operator=(NetworkedComponent&&) = default;

	};
}

#include <CommonLib/Components/NetworkedComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP
