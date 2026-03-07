// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_CAMERAFOLLOWERSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_CAMERAFOLLOWERSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class NodeComponent;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API CameraFollowerSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = -1'000'000;
			using Components = Nz::TypeList<class CameraFollowerComponent, Nz::NodeComponent>;

			inline CameraFollowerSystem(entt::registry& registry, float maxDistance);
			CameraFollowerSystem(const CameraFollowerSystem&) = delete;
			CameraFollowerSystem(CameraFollowerSystem&&) = delete;
			~CameraFollowerSystem() = default;

			void SetCameraPosition(const Nz::Vector3f& cameraPosition);

			void Update(Nz::Time elapsedTime);

			CameraFollowerSystem& operator=(const CameraFollowerSystem&) = delete;
			CameraFollowerSystem& operator=(CameraFollowerSystem&&) = delete;

		private:
			entt::registry& m_registry;
			Nz::Vector3f m_cameraPosition;
			float m_maxDistance;
	};
}

#include <ClientLib/Systems/CameraFollowerSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_CAMERAFOLLOWERSYSTEM_HPP
