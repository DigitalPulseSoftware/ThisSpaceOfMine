// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/GpuTransferAppComponent.hpp>
#include <Nazara/Renderer/GpuDevice.hpp>
#include <NazaraUtils/StackVector.hpp>

namespace tsom
{
	void GpuTransferAppComponent::RecordAsyncCommands(const Nz::FunctionRef<void(Nz::GpuAsyncCommands& asyncTransfer)>& callback)
	{
		std::unique_lock lock(m_mutex);

		std::unique_ptr<Nz::GpuAsyncCommands>& asyncCommands = m_asyncCommands.GetOrCreate();
		if (!asyncCommands)
			asyncCommands = m_gpuDevice.InstantiateAsyncCommands(Nz::QueueType::Transfer);

		callback(*asyncCommands);
	}

	void GpuTransferAppComponent::Update(Nz::Time elapsedTime)
	{
		m_broadcastTimer -= elapsedTime;
		if (m_broadcastTimer >= Nz::Time::Zero())
			return;

		m_broadcastTimer = Nz::Time::Milliseconds(100);

		Nz::HybridVector<std::unique_ptr<Nz::GpuAsyncCommands>, 64> asyncCommands;

		std::unique_lock lock(m_mutex);
		m_asyncCommands.Iterate([&](std::thread::id /*threadId*/, std::unique_ptr<Nz::GpuAsyncCommands>& asyncCommand)
		{
			if (asyncCommand)
			{
				asyncCommands.emplace_back(std::move(asyncCommand));
				asyncCommand.reset();
			}
		});

		if (!asyncCommands.empty())
			m_gpuDevice.SubmitAsyncCommands(std::span(asyncCommands.begin(), asyncCommands.end()));
	}
}
