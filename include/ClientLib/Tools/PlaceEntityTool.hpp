// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLS_PLACEENTITYTOOL_HPP
#define TSOM_CLIENTLIB_TOOLS_PLACEENTITYTOOL_HPP

#include <ClientLib/Tools/ToolBase.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <entt/entt.hpp>
#include <memory>
#include <optional>
#include <string_view>

namespace Nz
{
	class MaterialInstance;
}

namespace tsom
{
	class ClientAssetLibraryAppComponent;

	class TSOM_CLIENTLIB_API PlaceEntityTool final : public ToolBase
	{
		public:
			inline PlaceEntityTool(GameInterface& gameInterface, ClientAssetLibraryAppComponent& assetLibrary);

			void OnActivate() override;
			void OnDeactivate() override;

			void OnTrigger(bool primary) override;

			void Update(Nz::Time elapsedTime, const GameInterface::RaycastResult* previewRaycast) override;

		private:
			struct PreviewData
			{
				std::shared_ptr<Nz::MaterialInstance> material;
				entt::handle entity;
				Nz::UInt8 rotationMultiplier = 0; // * 45°
				Nz::Vector3f collider;
			};

			std::optional<PreviewData> m_preview;
			std::string_view m_selectedEntityClass;
			ClientAssetLibraryAppComponent& m_assetLibrary;
			bool m_isSelectingEntities;

	};
}

#include <ClientLib/Tools/PlaceEntityTool.inl>

#endif // TSOM_CLIENTLIB_TOOLS_PLACEENTITYTOOL_HPP
