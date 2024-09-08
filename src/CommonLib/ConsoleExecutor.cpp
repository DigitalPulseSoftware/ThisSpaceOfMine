// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ConsoleExecutor.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <NazaraUtils/CallOnExit.hpp>

namespace tsom
{
	ConsoleExecutor::ConsoleExecutor(ScriptingContext& scriptingContext) :
	m_scriptingContext(scriptingContext)
	{
	}

	void ConsoleExecutor::Execute(std::string_view str, const std::string& origin)
	{
		auto prevCallback = m_scriptingContext.OverridePrintCallback([this](std::string&& output)
		{
			OnOutput(this, output);
		});
		NAZARA_DEFER(m_scriptingContext.OverridePrintCallback(std::move(prevCallback)););

		auto res = m_scriptingContext.Execute(str, origin);
		if (!res)
			OnError(this, res.GetError());
	}
}
