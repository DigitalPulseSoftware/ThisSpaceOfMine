// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/ServerEnvironment.hpp>

namespace tsom
{
	class ChunkEntities;
	class Ship;

	class TSOM_SERVERLIB_API ServerShipEnvironment : public ServerEnvironment
	{
		public:
			ServerShipEnvironment(ServerInstance& serverInstance);
			ServerShipEnvironment(const ServerShipEnvironment&) = delete;
			ServerShipEnvironment(ServerShipEnvironment&&) = delete;
			~ServerShipEnvironment();

			entt::handle CreateEntity() override;

			const GravityController* GetGravityController() const override;
			Ship& GetShip();
			const Ship& GetShip() const;
			inline entt::handle GetShipEntity() const;

			void OnLoad(const std::filesystem::path& loadPath) override;
			void OnSave(const std::filesystem::path& savePath) override;

			ServerShipEnvironment& operator=(const ServerShipEnvironment&) = delete;
			ServerShipEnvironment& operator=(ServerShipEnvironment&&) = delete;

		private:
			entt::handle m_shipEntity;
	};
}

#include <ServerLib/ServerShipEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERSHIPENVIRONMENT_HPP
