// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SERVERWORLDAPPCOMPONENT_HPP
#define TSOM_COMMONLIB_SERVERWORLDAPPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ServerWorld.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API ServerWorldAppComponent : public Nz::ApplicationComponent
	{
		public:
			using ApplicationComponent::ApplicationComponent;
			ServerWorldAppComponent(const ServerWorld&) = delete;
			ServerWorldAppComponent(ServerWorld&&) = delete;
			~ServerWorldAppComponent() = default;

			template<typename... Args> ServerWorld& AddWorld(Args&&... args);

			void Update(Nz::Time elapsedTime) override;
			
			ServerWorldAppComponent& operator=(const ServerWorldAppComponent&) = delete;
			ServerWorldAppComponent& operator=(ServerWorldAppComponent&&) = delete;

		private:
			std::vector<std::unique_ptr<ServerWorld>> m_worlds;
	};
}

#include <CommonLib/ServerWorldAppComponent.inl>

#endif
