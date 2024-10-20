// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTENTERTRIGGERCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTENTERTRIGGERCOMPONENT_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Math/Box.hpp>
#include <memory>

namespace Nz
{
	class Collider3D;
}

namespace tsom
{
	class ServerEnvironment;

	struct EnvironmentEnterTriggerComponent
	{
		std::shared_ptr<Nz::Collider3D> entryTrigger;
		Nz::Boxf aabb; //< in trigger space, serves as a cheap test before testing entryTrigger
		ServerEnvironment* targetEnvironment;
	};
}

#include <ServerLib/Components/EnvironmentEnterTriggerComponent.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTENTERTRIGGERCOMPONENT_HPP
