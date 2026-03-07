// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	struct EnvironmentComponent
	{
		Packets::Helper::EnvironmentId environmentIndex;
	};
}

#endif // TSOM_CLIENTLIB_COMPONENTS_ENVIRONMENTCOMPONENT_HPP
