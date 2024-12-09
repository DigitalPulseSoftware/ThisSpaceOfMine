// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SHIPCONTROLLER_HPP
#define TSOM_COMMONLIB_SHIPCONTROLLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityReference.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class CharacterController;

	class TSOM_COMMONLIB_API ShipController
	{
		public:
			inline ShipController(EntityReference entity, const Nz::Quaternionf& rotation);
			ShipController(const ShipController&) = delete;
			ShipController(ShipController&&) = delete;
			~ShipController() = default;

			inline const Nz::Quaternionf& GetReferenceRotation() const;
			inline entt::handle GetShipEntity() const;

			void PostSimulate(CharacterController& character, float elapsedTime);
			void PreSimulate(CharacterController& character, float elapsedTime);

			ShipController& operator=(const ShipController&) = delete;
			ShipController& operator=(ShipController&&) = delete;

		private:
			EntityReference m_entity;
			Nz::Quaternionf m_rotation;
	};
}

#include <CommonLib/ShipController.inl>

#endif // TSOM_COMMONLIB_SHIPCONTROLLER_HPP
