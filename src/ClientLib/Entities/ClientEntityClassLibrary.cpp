// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>

namespace tsom
{
	void ClientEntityClassLibrary::OnPlayerActivate(entt::handle entity)
	{
	}

	void ClientEntityClassLibrary::OnPlayerRpc_Death(entt::handle entity)
	{
		fmt::print("YOU DIED\n");
	}
}
