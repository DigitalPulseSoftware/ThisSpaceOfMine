// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_BLOCKTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_BLOCKTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	class BlockSelectionBar;

	class TSOM_CLIENTLIB_API BlockTool final : public ToolBase
	{
		public:
			inline BlockTool(GameInterface& gameInterface);

			void OnTrigger(TriggerType triggerType) override;
			void OnWheel(float delta) override;

			void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast) override;
	};
}

#include <ClientLib/Tools/BlockTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_BLOCKTOOL_HPP
