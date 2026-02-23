// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline sol::state& ScriptingContext::GetState()
	{
		return m_state;
	}

	inline const sol::state& ScriptingContext::GetState() const
	{
		return m_state;
	}

	template<typename T, typename... Args>
	T& ScriptingContext::RegisterLibrary(Args&&... args)
	{
		return static_cast<T&>(RegisterLibrary(std::make_unique<T>(std::forward<Args>(args)...)));
	}
}
