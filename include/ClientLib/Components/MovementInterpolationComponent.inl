// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
	inline MovementInterpolationComponent::MovementInterpolationComponent(Nz::UInt16 lastTickIndex) :
	m_lastTickIndex(lastTickIndex),
	m_delta(0.f)
	{
	}

	inline bool MovementInterpolationComponent::Advance(float deltaIncrement, std::size_t targetMovementPoints, Nz::Vector3f* position, Nz::Quaternionf* rotation)
	{
		deltaIncrement *= 1.f + (0.05f * (float(m_movementPoints.size()) - float(targetMovementPoints)));

		m_delta += deltaIncrement;
		while (m_delta >= 1.f)
		{
			if NAZARA_UNLIKELY(!m_movementPoints.empty())
				m_movementPoints.erase(m_movementPoints.begin());

			m_delta -= 1.f;
		}

		if NAZARA_UNLIKELY(m_movementPoints.size() < 2)
			return false;

		*position = Nz::Vector3f::Lerp(m_movementPoints[0].position, m_movementPoints[1].position, m_delta);
		*rotation = Nz::Quaternionf::Slerp(m_movementPoints[0].rotation, m_movementPoints[1].rotation, m_delta);

		return true;
	}

	inline void MovementInterpolationComponent::Fill(std::size_t count, const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		assert(count < m_movementPoints.capacity());

		m_movementPoints.clear();
		m_movementPoints.resize(count, {
			rotation,
			position
		});
	}

	inline std::size_t MovementInterpolationComponent::GetMovementPointCount() const
	{
		return m_movementPoints.size();
	}

	inline void MovementInterpolationComponent::PushMovement(Nz::UInt16 tickIndex, const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		Nz::UInt16 tickDifference = tickIndex - m_lastTickIndex;
		m_lastTickIndex = tickIndex;

		// If we lost a movement, build missing movements by interpolating
		if (tickDifference >= 2 && !m_movementPoints.empty())
		{
			auto& referencePosition = m_movementPoints.back();

			std::size_t interpolatedPosition = tickDifference - 1;
			float deltaIncr = 1.f / tickDifference;

			float delta = 0.f;
			for (std::size_t i = 0; i < interpolatedPosition; ++i)
			{
				delta += deltaIncr;

				Nz::Vector3f interpPosition = Nz::Vector3f::Lerp(referencePosition.position, position, delta);
				Nz::Quaternionf interpRotation = Nz::Quaternionf::Slerp(referencePosition.rotation, rotation, delta);

				PushMovement(interpPosition, interpRotation);
			}
		}

		PushMovement(position, rotation);
	}

	inline void MovementInterpolationComponent::PushMovement(const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		if (m_movementPoints.size() >= m_movementPoints.capacity())
		{
			m_movementPoints.back() = {
				rotation,
				position
			};
		}
		else
		{
			m_movementPoints.push_back({
				rotation,
				position
			});
		}
	}
}
