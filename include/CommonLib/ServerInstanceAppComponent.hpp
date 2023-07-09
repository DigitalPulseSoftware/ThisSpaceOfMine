// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SERVERINSTANCEAPPCOMPONENT_HPP
#define TSOM_COMMONLIB_SERVERINSTANCEAPPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ServerInstance.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API ServerInstanceAppComponent : public Nz::ApplicationComponent
	{
		public:
			using ApplicationComponent::ApplicationComponent;
			ServerInstanceAppComponent(const ServerInstance&) = delete;
			ServerInstanceAppComponent(ServerInstance&&) = delete;
			~ServerInstanceAppComponent() = default;

			template<typename... Args> ServerInstance& AddInstance(Args&&... args);

			void Update(Nz::Time elapsedTime) override;
			
			ServerInstanceAppComponent& operator=(const ServerInstanceAppComponent&) = delete;
			ServerInstanceAppComponent& operator=(ServerInstanceAppComponent&&) = delete;

		private:
			std::vector<std::unique_ptr<ServerInstance>> m_instances;
	};
}

#include <CommonLib/ServerInstanceAppComponent.inl>

#endif
