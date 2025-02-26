// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERATMOSPHERE_HPP
#define TSOM_SERVERLIB_SERVERATMOSPHERE_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/GasType.hpp>
#include <NazaraUtils/EnumArray.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerAtmosphere
	{
		public:
			inline ServerAtmosphere();
			ServerAtmosphere(const ServerAtmosphere&) = delete;
			ServerAtmosphere(ServerAtmosphere&&) = default;
			~ServerAtmosphere() = default;

			inline bool Exchange(const Nz::EnumArray<GasType, Nz::Int32>& amounts);

			inline Nz::UInt64 GetGasAmount(GasType type) const;

			inline void SetGasAmount(GasType type, Nz::UInt64 millilitre);

			ServerAtmosphere& operator=(const ServerAtmosphere&) = delete;
			ServerAtmosphere& operator=(ServerAtmosphere&&) = default;

		private:
			Nz::EnumArray<GasType, Nz::UInt64> m_gasAmount; //< mL
	};
}

#include <ServerLib/ServerAtmosphere.inl>

#endif // TSOM_SERVERLIB_SERVERATMOSPHERE_HPP
