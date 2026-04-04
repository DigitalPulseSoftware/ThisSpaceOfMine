// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTASSETCOOKER_HPP
#define TSOM_CLIENTLIB_CLIENTASSETCOOKER_HPP

#include <ClientLib/Export.hpp>
#include <NazaraUtils/Result.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class ClientBlockLibrary;

	class TSOM_CLIENTLIB_API ClientAssetCooker
	{
		public:
			inline ClientAssetCooker(Nz::ApplicationBase& app);
			ClientAssetCooker(const ClientAssetCooker&) = delete;
			ClientAssetCooker(ClientAssetCooker&&) = default;
			~ClientAssetCooker() = default;

			Nz::Result<void, std::string> Cook(ClientBlockLibrary& blockLibrary);

			ClientAssetCooker& operator=(const ClientAssetCooker&) = delete;
			ClientAssetCooker& operator=(ClientAssetCooker&&) = default;

		private:
			Nz::ApplicationBase& m_app;
	};
}

#include <ClientLib/ClientAssetCooker.inl>

#endif // TSOM_CLIENTLIB_CLIENTASSETCOOKER_HPP
