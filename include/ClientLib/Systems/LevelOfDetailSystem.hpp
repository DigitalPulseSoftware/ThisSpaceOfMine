// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_LEVELOFDETAILSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_LEVELOFDETAILSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/Signal.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class ConfigFile;

	class TSOM_CLIENTLIB_API LevelOfDetailSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = -2'000'000;
			using Components = Nz::TypeList<class ChunkContainerComponent>;

			LevelOfDetailSystem(entt::registry& registry, ConfigFile& configFile);
			LevelOfDetailSystem(const LevelOfDetailSystem&) = delete;
			LevelOfDetailSystem(LevelOfDetailSystem&&) = delete;
			~LevelOfDetailSystem() = default;

			void Update(Nz::Time elapsedTime);

			LevelOfDetailSystem& operator=(const LevelOfDetailSystem&) = delete;
			LevelOfDetailSystem& operator=(LevelOfDetailSystem&&) = delete;

		private:
			entt::registry& m_registry;
			Nz::Signal<long long>::ConnectionGuard m_onChunkMinLevelOfDetailUpdatedSlot;
			ConfigFile& m_configFile;
	};
}

#include <ClientLib/Systems/LevelOfDetailSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_LEVELOFDETAILSYSTEM_HPP
