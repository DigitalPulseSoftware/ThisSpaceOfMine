// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/ToolBase.hpp>

namespace tsom
{
	ToolBase::~ToolBase() = default;

	void ToolBase::OnActivate()
	{
	}

	void ToolBase::OnDeactivate()
	{
	}

	void ToolBase::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* /*previewRaycast*/)
	{
	}
}
