// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SERVERWORLD_HPP
#define TSOM_COMMONLIB_SERVERWORLD_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API ServerWorld
	{
		public:
			ServerWorld() = default;
			ServerWorld(const ServerWorld&) = delete;
			ServerWorld(ServerWorld&&) = delete;
			~ServerWorld() = default;

			template<typename... Args> NetworkSessionManager& AddSessionManager(Args&&... args);

			void Update(Nz::Time elapsedTime);

			ServerWorld& operator=(const ServerWorld&) = delete;
			ServerWorld& operator=(ServerWorld&&) = delete;

		private:
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
	};
}

#include <CommonLib/ServerWorld.inl>

#endif
