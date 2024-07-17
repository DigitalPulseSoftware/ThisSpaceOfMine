// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_MOVEMENTINTERPOLATIONCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_MOVEMENTINTERPOLATIONCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FixedVector.hpp>

namespace tsom
{
	class MovementInterpolationComponent
	{
		public:
			inline MovementInterpolationComponent(Nz::UInt16 lastTickIndex);
			MovementInterpolationComponent(const MovementInterpolationComponent&) = delete;
			MovementInterpolationComponent(MovementInterpolationComponent&&) = default;
			~MovementInterpolationComponent() = default;

			inline bool Advance(float deltaIncrement, std::size_t targetMovementPoints, Nz::Vector3f* position, Nz::Quaternionf* rotation);

			inline void Fill(std::size_t count, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			inline std::size_t GetMovementPointCount() const;

			inline void PushMovement(Nz::UInt16 tickIndex, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			MovementInterpolationComponent& operator=(const MovementInterpolationComponent&) = delete;
			MovementInterpolationComponent& operator=(MovementInterpolationComponent&&) = default;

			static constexpr std::size_t MaxPoint = 10;

		private:
			inline void PushMovement(const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			struct MovementData
			{
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
			};

			Nz::FixedVector<MovementData, MaxPoint> m_movementPoints;
			Nz::UInt16 m_lastTickIndex;
			float m_delta;
	};
}

#include <ClientLib/Components/MovementInterpolationComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_MOVEMENTINTERPOLATIONCOMPONENT_HPP
