// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline GpuTransferAppComponent::GpuTransferAppComponent(Nz::ApplicationBase& app, Nz::GpuDevice& gpuDevice) :
	ApplicationComponent(app),
	m_broadcastTimer(Nz::Time::Zero()),
	m_gpuDevice(gpuDevice)
	{
	}
}
