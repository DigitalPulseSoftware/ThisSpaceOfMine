// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERATMOSPHERE_HPP
#define TSOM_SERVERLIB_SERVERATMOSPHERE_HPP

#include <ServerLib/Export.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerAtmosphere
	{
		public:
			ServerAtmosphere() = default;
			ServerAtmosphere(const ServerAtmosphere&) = delete;
			ServerAtmosphere(ServerAtmosphere&&) = default;
			~ServerAtmosphere() = default;

			ServerAtmosphere& operator=(const ServerAtmosphere&) = delete;
			ServerAtmosphere& operator=(ServerAtmosphere&&) = default;

		private:
	};
}

#include <ServerLib/ServerAtmosphere.inl>

#endif // TSOM_SERVERLIB_SERVERATMOSPHERE_HPP
