// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_NETWORKINTERPOLATIONCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_NETWORKINTERPOLATIONCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FixedVector.hpp>

namespace Nz
{
	class Node;
}

namespace tsom
{
	class NetworkInterpolationComponent
	{
		public:
			inline NetworkInterpolationComponent(Nz::UInt16 lastTickIndex);
			NetworkInterpolationComponent(const NetworkInterpolationComponent&) = delete;
			NetworkInterpolationComponent(NetworkInterpolationComponent&&) = default;
			~NetworkInterpolationComponent() = default;

			inline bool Advance(float deltaIncrement, std::size_t targetMovementPoints, Nz::Vector3f* position, Nz::Quaternionf* rotation);

			inline void Fill(std::size_t count, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			inline std::size_t GetMovementPointCount() const;

			inline void PushMovement(Nz::UInt16 tickIndex, const Nz::Vector3f& position, const Nz::Quaternionf& rotation);

			inline void UpdateRoot(const Nz::Node& previousRoot, const Nz::Node& newRoot);

			NetworkInterpolationComponent& operator=(const NetworkInterpolationComponent&) = delete;
			NetworkInterpolationComponent& operator=(NetworkInterpolationComponent&&) = default;

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

#include <ClientLib/Components/NetworkInterpolationComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_NETWORKINTERPOLATIONCOMPONENT_HPP
