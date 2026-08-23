// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_TOOLBASE_HPP
#define TSOM_CLIENTLIB_TOOLS_TOOLBASE_HPP

#include <ClientLib/Export.hpp>
#include <ClientLib/GameInterface.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <string_view>

namespace tsom
{
	class TSOM_CLIENTLIB_API ToolBase
	{
		public:
			enum class TriggerType
			{
				Primary,
				Secondary,
				Tertiary
			};

			inline ToolBase(GameInterface& gameInterface, std::string toolName);
			ToolBase(const ToolBase&) = delete;
			ToolBase(ToolBase&&) = delete;
			virtual ~ToolBase();

			inline const std::string& GetName() const;

			inline bool IsCursorUnlocked() const;

			virtual void OnActivate();
			virtual void OnDeactivate();

			virtual void OnTrigger(TriggerType triggerType) = 0;
			virtual void OnWheel(float delta);

			virtual void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast);

			ToolBase& operator=(const ToolBase&) = delete;
			ToolBase& operator=(ToolBase&&) = delete;

		protected:
			std::string m_toolName;
			GameInterface& m_gameInterface;
			bool m_isCursorUnlocked;
	};
}

#include <ClientLib/Tools/ToolBase.inl>

#endif // TSOM_CLIENTLIB_TOOLS_TOOLBASE_HPP
