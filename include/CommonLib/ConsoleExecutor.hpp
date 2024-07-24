// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP
#define TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP

#include <NazaraUtils/Signal.hpp>
#include <CommonLib/Export.hpp>
#include <sol/sol.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ConsoleExecutor
	{
		public:
			ConsoleExecutor();
			ConsoleExecutor(const ConsoleExecutor&) = delete;
			ConsoleExecutor(ConsoleExecutor&&) = delete;
			~ConsoleExecutor() = default;

			void Execute(std::string_view str, const std::string& origin);

			ConsoleExecutor& operator=(const ConsoleExecutor&) = delete;
			ConsoleExecutor& operator=(ConsoleExecutor&&) = delete;

			NazaraSignal(OnError, ConsoleExecutor* /*executor*/, std::string_view /*error*/);
			NazaraSignal(OnOutput, ConsoleExecutor* /*executor*/, std::string_view /*output*/);

		private:
			sol::state m_state;
	};
}

#include <CommonLib/ConsoleExecutor.inl>

#endif // TSOM_COMMONLIB_CONSOLEEXECUTOR_HPP
