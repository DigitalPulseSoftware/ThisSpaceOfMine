// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_NOTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_NOTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API NoTool : public ToolBase
	{
		public:
			inline NoTool(GameInterface& gameInterface);

			void OnTrigger(TriggerType triggerType) override;
	};
}

#include <ClientLib/Tools/NoTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_NOTOOL_HPP
