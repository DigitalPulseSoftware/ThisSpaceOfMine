// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_CHUNKENTITIESSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_CHUNKENTITIESSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ChunkEntitiesSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = -1'000'000;
			using Components = Nz::TypeList<class ChunkContainerComponent>;

			inline ChunkEntitiesSystem(entt::registry& registry);
			ChunkEntitiesSystem(const ChunkEntitiesSystem&) = delete;
			ChunkEntitiesSystem(ChunkEntitiesSystem&&) = delete;
			~ChunkEntitiesSystem() = default;

			void Update(Nz::Time elapsedTime);

			ChunkEntitiesSystem& operator=(const ChunkEntitiesSystem&) = delete;
			ChunkEntitiesSystem& operator=(ChunkEntitiesSystem&&) = delete;

		private:
			entt::registry& m_registry;
	};
}

#include <CommonLib/Systems/ChunkEntitiesSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_CHUNKENTITIESSYSTEM_HPP
