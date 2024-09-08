// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <sol/state.hpp>

namespace tsom
{
	void ServerScriptingLibrary::Register(sol::state& state)
	{
		state["CLIENT"] = false;
		state["SERVER"] = true;
	}
}
