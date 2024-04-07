// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP

namespace tsom
{
	class NetworkedComponent
	{
		public:
			inline NetworkedComponent(bool shouldSignalCreation = true);
			NetworkedComponent(const NetworkedComponent&) = delete;
			NetworkedComponent(NetworkedComponent&&) = default;
			~NetworkedComponent() = default;

			inline bool ShouldSignalCreation() const;

			NetworkedComponent& operator=(const NetworkedComponent&) = delete;
			NetworkedComponent& operator=(NetworkedComponent&&) = default;

		private:
			bool m_shouldSignalCreation;
	};
}

#include <ServerLib/Components/NetworkedComponent.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_NETWORKEDCOMPONENT_HPP
