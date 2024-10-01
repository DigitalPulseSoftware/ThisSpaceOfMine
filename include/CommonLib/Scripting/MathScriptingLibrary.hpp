// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_MATHSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_MATHSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API MathScriptingLibrary : public ScriptingLibrary
	{
		public:
			MathScriptingLibrary() = default;
			MathScriptingLibrary(const MathScriptingLibrary&) = delete;
			MathScriptingLibrary(MathScriptingLibrary&&) = delete;
			~MathScriptingLibrary() = default;

			void Register(sol::state& state) override;

			MathScriptingLibrary& operator=(const MathScriptingLibrary&) = delete;
			MathScriptingLibrary& operator=(MathScriptingLibrary&&) = delete;

		private:
			void RegisterBox(sol::state& state);
			void RegisterEulerAngles(sol::state& state);
			void RegisterQuaternion(sol::state& state);
			void RegisterVector2(sol::state& state);
			void RegisterVector3(sol::state& state);
	};
}

#include <CommonLib/Scripting/MathScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_MATHSCRIPTINGLIBRARY_HPP
