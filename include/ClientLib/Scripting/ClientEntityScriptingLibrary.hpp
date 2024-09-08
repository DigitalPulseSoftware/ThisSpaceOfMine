// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SCRIPTING_CLIENTENTITYSCRIPTINGLIBRARY_HPP
#define TSOM_CLIENTLIB_SCRIPTING_CLIENTENTITYSCRIPTINGLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Scripting/EntityScriptingLibrary.hpp>
#include <sol/state.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientEntityScriptingLibrary : public EntityScriptingLibrary
	{
		public:
			using EntityScriptingLibrary::EntityScriptingLibrary;

			void Register(sol::state& state) override;

		private:
			void FillConstants(sol::state& state, sol::table constants) override;
			void RegisterClientComponents(sol::state& state);

			AddComponentFunc RetrieveAddComponentHandler(std::string_view componentType) override;
			GetComponentFunc RetrieveGetComponentHandler(std::string_view componentType) override;
	};
}

#include <ClientLib/Scripting/ClientEntityScriptingLibrary.inl>

#endif // TSOM_CLIENTLIB_SCRIPTING_CLIENTENTITYSCRIPTINGLIBRARY_HPP
