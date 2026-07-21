// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_CONNECTTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_CONNECTTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <Nazara/Core/Color.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API ConnectTool final : public ToolBase
	{
		public:
			inline ConnectTool(GameInterface& gameInterface);
		
			void OnTrigger(bool primary) override;

			void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast) override;

		private:
			void DrawEntityAABB(entt::handle entity, Nz::Color color);

			entt::handle m_hoveredEntity;
			entt::handle m_selectedSourceEntity;
	};
}

#include <ClientLib/Tools/ConnectTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_CONNECTTOOL_HPP
