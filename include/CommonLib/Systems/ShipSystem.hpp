// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP

#include <CommonLib/Export.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ShipSystem
	{
		public:
			ShipSystem() = default;
			ShipSystem(const ShipSystem&) = delete;
			ShipSystem(ShipSystem&&) = delete;
			~ShipSystem() = default;

			ShipSystem& operator=(const ShipSystem&) = delete;
			ShipSystem& operator=(ShipSystem&&) = delete;

		private:
	};
}

#include <CommonLib/Systems/ShipSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP
