// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_PLAYERANIMATIONCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_PLAYERANIMATIONCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Animation.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <memory>

namespace Nz
{
	class Skeleton;
}

namespace tsom
{
	class PlayerAnimationComponent
	{
		public:
			inline PlayerAnimationComponent(std::shared_ptr<Nz::Skeleton> skeleton, std::shared_ptr<Nz::Animation> currentAnim);
			PlayerAnimationComponent(const PlayerAnimationComponent&) = default;
			PlayerAnimationComponent(PlayerAnimationComponent&&) = default;
			~PlayerAnimationComponent() = default;

			inline void Advance(Nz::Time elapsedTime);

			PlayerAnimationComponent& operator=(const PlayerAnimationComponent&) = delete;
			PlayerAnimationComponent& operator=(PlayerAnimationComponent&&) = default;

		private:
			std::shared_ptr<Nz::Animation> m_currentAnim;
			std::shared_ptr<Nz::Skeleton> m_skeleton;
			std::size_t m_currentFrame;
			std::size_t m_previousFrame;
			float m_interp;
	};
}

#include <ClientLib/Components/PlayerAnimationComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_PLAYERANIMATIONCOMPONENT_HPP
