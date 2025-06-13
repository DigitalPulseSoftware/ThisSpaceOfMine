// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_CRASHHANDLER_HPP
#define TSOM_COMMONLIB_UTILITY_CRASHHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <cpptrace/forward.hpp>
#include <iosfwd>
#include <memory>

namespace tsom
{
	class TSOM_COMMONLIB_API CrashHandler
	{
		public:
			CrashHandler() = default;
			virtual ~CrashHandler();

			virtual bool Install() = 0;

			virtual void HandleUnhandledException(const std::exception* e, const cpptrace::stacktrace& stacktrace) = 0;

			virtual void Uninstall() = 0;

			static std::unique_ptr<CrashHandler> PlatformCrashHandler();

		protected:
			virtual void WriteHeader(std::ostream& filestream);
	};
}

#include <CommonLib/Utility/CrashHandler.inl>

#endif // TSOM_COMMONLIB_UTILITY_CRASHHANDLER_HPP
