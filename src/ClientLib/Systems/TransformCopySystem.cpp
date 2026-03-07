// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/TransformCopySystem.hpp>
#include <ClientLib/Components/TransformCopyComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	void TransformCopySystem::Update(Nz::Time /*elapsedTime*/)
	{
		auto view = m_registry.view<Nz::NodeComponent, TransformCopyComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, entityNode, referencedTransform] : view.each())
		{
			auto& targetNode = referencedTransform.referenceEntity.get<Nz::NodeComponent>();
			entityNode.CopyLocalTransform(targetNode);
		}
	}
}
