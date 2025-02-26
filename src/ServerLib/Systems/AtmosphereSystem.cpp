// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/AtmosphereSystem.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <ServerLib/ServerAtmosphere.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/Components/AtmosphereCarrier.hpp>
#include <ServerLib/Components/AtmosphereExchanger.hpp>
#include <ServerLib/Components/AtmosphereMonitor.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <fmt/format.h>

namespace tsom
{
	AtmosphereSystem::AtmosphereSystem(entt::registry& registry) :
	m_registry(registry)
	{
		m_ownerEnvironment = m_registry.ctx().get<ServerEnvironment*>();
	}

	void AtmosphereSystem::Update(Nz::Time elapsedTime)
	{
		UpdateAtmosphere();
		PerformExchange(elapsedTime);
	}

	void AtmosphereSystem::PerformExchange(Nz::Time elapsedTime)
	{
		auto exchangeView = m_registry.view<AtmosphereMonitor, AtmosphereExchanger>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [exchangerEntity, exchangerMonitor, exchangerData] : exchangeView.each())
		{
			exchangerData.timeBeforeTick -= elapsedTime;
			if (exchangerData.timeBeforeTick > Nz::Time::Zero())
				continue;

			exchangerData.timeBeforeTick += exchangerData.tickRate;

			if (!exchangerMonitor.atmosphere)
			{
				// If we're in a vaccum there's no possible exchange
				exchangerData.OnExchangeFailed(entt::handle(m_registry, exchangerEntity), &exchangerData);
				continue;
			}

			if (!exchangerMonitor.atmosphere->Exchange(exchangerData.gasModifier))
			{
				// If we're in a vaccum there's no possible exchange
				exchangerData.OnExchangeFailed(entt::handle(m_registry, exchangerEntity), &exchangerData);
				continue;
			}

			// We're in an atmosphere
			exchangerData.OnExchange(entt::handle(m_registry, exchangerEntity), &exchangerData);
		}
	}

	void AtmosphereSystem::UpdateAtmosphere()
	{
		auto monitorView = m_registry.view<Nz::NodeComponent, AtmosphereMonitor>(entt::exclude<Nz::DisabledComponent>);
		auto carrierView = m_registry.view<Nz::NodeComponent, AtmosphereCarrier>(entt::exclude<Nz::DisabledComponent>);

		for (auto&& [monitorEntity, monitorNode, monitor] : monitorView.each())
		{
			Nz::Vector3f monitorPosition = monitorNode.GetPosition();

			ServerAtmosphere* previousAtmosphere = monitor.atmosphere;

			monitor.atmosphere = nullptr;
			for (auto&& [carrierEntity, carrierNode, carrier] : carrierView.each())
			{
				Nz::Vector3f localPos = carrierNode.ToLocalPosition(monitorPosition);

				// Use AABB as a cheap test
				if NAZARA_LIKELY(!carrier.aabb.Contains(localPos))
					continue;

				if (carrier.collider)
				{
					if (!carrier.collider->CollisionQuery(localPos - carrier.collider->GetCenterOfMass())) //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
						continue;
				}

				monitor.atmosphere = carrier.atmosphere;
				break;
			}

			if (!monitor.atmosphere)
				monitor.atmosphere = m_ownerEnvironment->GetAtmosphereAtPosition(monitorPosition);

			if (monitor.atmosphere != previousAtmosphere)
				fmt::print("atmosphere update: {}!\n", (void*) monitor.atmosphere);
		}
	}
}
