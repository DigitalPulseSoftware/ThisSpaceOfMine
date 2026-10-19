// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERSPACEENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERSPACEENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerSpaceEnvironment final : public ServerEnvironment
	{
		public:
			ServerSpaceEnvironment(ServerInstance& serverInstance, const Nz::Vector3i32& coordinates);
			ServerSpaceEnvironment(const ServerSpaceEnvironment&) = delete;
			ServerSpaceEnvironment(ServerSpaceEnvironment&&) = delete;
			~ServerSpaceEnvironment() = default;

			Nz::Boxf ComputeBoundingBox() const override;

			entt::handle CreateEntity() override;

			ChunkContainer* GetChunkContainer() override;
			const ChunkContainer* GetChunkContainer() const override;
			inline const Nz::Vector3i32& GetCoordinates() const;
			const GravityController* GetGravityController() const override;

			void OnSave() override;
			void OnTick(Nz::Time elapsedTime) override;

			ServerSpaceEnvironment& operator=(const ServerSpaceEnvironment&) = delete;
			ServerSpaceEnvironment& operator=(ServerSpaceEnvironment&&) = delete;

		private:
			ServerAtmosphere* GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position) override;

			Nz::Vector3i32 m_coordinates;
	};
}

#include <ServerLib/ServerSpaceEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERSPACEENVIRONMENT_HPP
