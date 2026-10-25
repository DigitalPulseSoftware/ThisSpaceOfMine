// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_AUDIOLISTENERCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_AUDIOLISTENERCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	struct AudioListenerComponent
	{
		Nz::Vector3f lastPosition; //< To compute velocity
	};
}

#include <ClientLib/Components/AudioListenerComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_AUDIOLISTENERCOMPONENT_HPP
