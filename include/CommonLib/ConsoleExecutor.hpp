// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP
#define TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/Signal.hpp>
#include <string>

namespace tsom
{
	class ScriptingContext;

	class TSOM_COMMONLIB_API ConsoleExecutor
	{
		public:
			ConsoleExecutor(ScriptingContext& scriptingContext);
			ConsoleExecutor(const ConsoleExecutor&) = delete;
			ConsoleExecutor(ConsoleExecutor&&) = delete;
			~ConsoleExecutor() = default;

			void Execute(std::string_view str, const std::string& origin);

			ConsoleExecutor& operator=(const ConsoleExecutor&) = delete;
			ConsoleExecutor& operator=(ConsoleExecutor&&) = delete;

			NazaraSignal(OnError, ConsoleExecutor* /*executor*/, std::string_view /*error*/);
			NazaraSignal(OnOutput, ConsoleExecutor* /*executor*/, std::string_view /*output*/);

		private:
			ScriptingContext& m_scriptingContext;
	};
}

#include <CommonLib/ConsoleExecutor.inl>

#endif // TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP
