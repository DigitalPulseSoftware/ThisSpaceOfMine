// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREEXCHANGER_HPP
#define TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREEXCHANGER_HPP

#include <CommonLib/GasType.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <NazaraUtils/Signal.hpp>

namespace tsom
{
	struct AtmosphereExchanger
	{
		AtmosphereExchanger()
		{
			gasModifier.fill(0);
		}

		Nz::EnumArray<GasType, Nz::Int32> gasModifier;
		Nz::Time timeBeforeTick = Nz::Time::Zero();
		Nz::Time tickRate = Nz::Time::Second();

		NazaraSignal(OnExchangeFailed, entt::handle /*entity*/, AtmosphereExchanger* /*exchanger*/);
		NazaraSignal(OnExchange, entt::handle /*entity*/, AtmosphereExchanger* /*exchanger*/);
	};
}

#include <ServerLib/Components/AtmosphereExchanger.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREEXCHANGER_HPP
