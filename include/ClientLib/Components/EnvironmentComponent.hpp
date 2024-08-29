// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <cstdint>

namespace tsom
{
	struct EnvironmentComponent
	{
		std::size_t environmentIndex;
	};
}

#include <ClientLib/Components/EnvironmentComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP
