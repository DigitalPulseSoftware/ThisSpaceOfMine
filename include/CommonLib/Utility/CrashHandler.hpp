// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CRASHHANDLER_HPP
#define TSOM_COMMONLIB_CRASHHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <memory>

namespace tsom
{
	class TSOM_COMMONLIB_API CrashHandler
	{
		public:
			CrashHandler() = default;
			virtual ~CrashHandler();

			virtual bool Install() = 0;
			virtual void Uninstall() = 0;

			static std::unique_ptr<CrashHandler> PlatformCrashHandler();
	};
}

#include <CommonLib/Utility/CrashHandler.inl>

#endif
