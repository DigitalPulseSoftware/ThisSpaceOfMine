// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ANIMATIONCONTROLLER_HPP
#define TSOM_CLIENTLIB_ANIMATIONCONTROLLER_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class Skeleton;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API AnimationController
	{
		public:
			AnimationController() = default;
			AnimationController(const AnimationController&) = delete;
			AnimationController(AnimationController&&) = delete;
			virtual ~AnimationController();

			virtual void Animate(Nz::Skeleton& skeleton, Nz::Time elapsedTime) = 0;

			virtual void UpdateStates(Nz::Time elapsedTime) = 0;

			AnimationController& operator=(const AnimationController&) = delete;
			AnimationController& operator=(AnimationController&&) = delete;
	};
}

#include <ClientLib/AnimationController.inl>

#endif // TSOM_CLIENTLIB_ANIMATIONCONTROLLER_HPP
