// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_GAMECONFIGAPPCOMPONENT_HPP
#define TSOM_GAME_GAMECONFIGAPPCOMPONENT_HPP

#include <CommonLib/ConfigFile.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	class GameConfigFile : public ConfigFile
	{
		public:
			GameConfigFile();
	};

	class GameConfigAppComponent final : public Nz::ApplicationComponent
	{
		public:
			GameConfigAppComponent(Nz::ApplicationBase& app);
			GameConfigAppComponent(const GameConfigAppComponent&) = delete;
			GameConfigAppComponent(GameConfigAppComponent&&) = delete;
			~GameConfigAppComponent() = default;

			inline GameConfigFile& GetConfig();
			inline const GameConfigFile& GetConfig() const;

			void Save();

			GameConfigAppComponent& operator=(const GameConfigAppComponent&) = delete;
			GameConfigAppComponent& operator=(GameConfigAppComponent&&) = delete;

		private:
			GameConfigFile m_configFile;
	};
}

#include <Game/GameConfigAppComponent.inl>

#endif // TSOM_GAME_GAMECONFIGAPPCOMPONENT_HPP
