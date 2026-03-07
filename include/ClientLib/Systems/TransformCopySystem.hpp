// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_TRANSFORMCOPYSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_TRANSFORMCOPYSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class NodeComponent;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API TransformCopySystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 2; //< after physics and interpolation
			using Components = Nz::TypeList<class TransformCopyComponent, Nz::NodeComponent>;

			inline TransformCopySystem(entt::registry& registry);
			TransformCopySystem(const TransformCopySystem&) = delete;
			TransformCopySystem(TransformCopySystem&&) = delete;
			~TransformCopySystem() = default;

			void Update(Nz::Time elapsedTime);

			TransformCopySystem& operator=(const TransformCopySystem&) = delete;
			TransformCopySystem& operator=(TransformCopySystem&&) = delete;

		private:
			entt::registry& m_registry;
	};
}

#include <ClientLib/Systems/TransformCopySystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_TRANSFORMCOPYSYSTEM_HPP
