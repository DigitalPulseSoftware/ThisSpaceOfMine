// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	PlayerSessionHandler::PlayerSessionHandler()
	{
		SetupHandlerTable<PlayerSessionHandler>();
	}

	void PlayerSessionHandler::HandlePacket(Packets::Test&& test)
	{
		fmt::print("Received Test: {}\n", test.str);
	}
}
