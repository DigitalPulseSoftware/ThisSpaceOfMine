// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_CRASHHANDLERFALLBACK_HPP
#define TSOM_COMMONLIB_UTILITY_CRASHHANDLERFALLBACK_HPP

#include <CommonLib/Utility/CrashHandler.hpp>
#include <memory>

namespace tsom
{
	class CrashHandlerFallback : public CrashHandler
	{
		public:
			CrashHandlerFallback() = default;
			~CrashHandlerFallback() = default;

			bool Install() override;
			void Uninstall() override;
	};
}

#include <CommonLib/Utility/CrashHandlerFallback.inl>

#endif // TSOM_COMMONLIB_UTILITY_CRASHHANDLERFALLBACK_HPP
