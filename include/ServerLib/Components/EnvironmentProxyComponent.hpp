// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTPROXYCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTPROXYCOMPONENT_HPP

#include <ServerLib/Export.hpp>

namespace tsom
{
	class ServerEnvironment;

	struct EnvironmentProxyComponent
	{
		ServerEnvironment* fromEnv;
		ServerEnvironment* toEnv;
	};
}

#include <ServerLib/Components/EnvironmentProxyComponent.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_ENVIRONMENTPROXYCOMPONENT_HPP
