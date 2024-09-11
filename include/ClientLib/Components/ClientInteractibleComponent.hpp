// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_CLIENTINTERACTIBLECOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_CLIENTINTERACTIBLECOMPONENT_HPP

#include <string>

namespace tsom
{
	struct ClientInteractibleComponent
	{
		std::string interactText;
		bool isEnabled = true;
	};
}

#endif // TSOM_CLIENTLIB_COMPONENTS_CLIENTINTERACTIBLECOMPONENT_HPP
