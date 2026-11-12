// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_GPUTRANSFERAPPCOMPONENT_HPP
#define TSOM_CLIENTLIB_GPUTRANSFERAPPCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <Nazara/Core/ThreadLocalData.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <mutex>

namespace Nz
{
	class GpuAsyncCommands;
	class GpuDevice;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API GpuTransferAppComponent : public Nz::ApplicationComponent
	{
		public:
			inline GpuTransferAppComponent(Nz::ApplicationBase& app, Nz::GpuDevice& gpuDevice);

			void RecordAsyncCommands(const Nz::FunctionRef<void(Nz::GpuAsyncCommands& asyncTransfer)>& callback);

		private:
			void Update(Nz::Time elapsedTime) override;

			Nz::ThreadLocalData<std::unique_ptr<Nz::GpuAsyncCommands>> m_asyncCommands;
			Nz::Time m_broadcastTimer;
			Nz::GpuDevice& m_gpuDevice;
			std::mutex m_mutex;
	};
}

#include <ClientLib/GpuTransferAppComponent.inl>

#endif // TSOM_CLIENTLIB_GPUTRANSFERAPPCOMPONENT_HPP
