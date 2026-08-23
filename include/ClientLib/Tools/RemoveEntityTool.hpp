// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_REMOVEENTITYTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_REMOVEENTITYTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API RemoveEntityTool final : public ToolBase
	{
		public:
			inline RemoveEntityTool(GameInterface& gameInterface);

			void OnTrigger(bool primary) override;

			void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast) override;

		private:
			entt::handle m_targetEntity;
	};
}

#include <ClientLib/Tools/RemoveEntityTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_REMOVEENTITYTOOL_HPP
