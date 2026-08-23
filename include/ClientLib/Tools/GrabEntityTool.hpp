// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_GRABENTITYTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_GRABENTITYTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API GrabEntityTool final : public ToolBase
	{
		public:
			inline GrabEntityTool(GameInterface& gameInterface);

			void OnTrigger(TriggerType triggerType) override;
	};
}

#include <ClientLib/Tools/GrabEntityTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_GRABENTITYTOOL_HPP
