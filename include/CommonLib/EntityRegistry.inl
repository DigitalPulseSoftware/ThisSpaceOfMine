// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline auto EntityRegistry::FindClass(std::string_view entityClass) const -> const EntityClass*
	{
		auto it = m_classes.find(entityClass);
		if (it == m_classes.end())
			return nullptr;

		return &it->second;
	}

	template<typename F>
	void EntityRegistry::ForEachClass(F&& functor)
	{
		for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
			functor(it.key(), it.value());
	}

	template<typename F>
	void EntityRegistry::ForEachClass(F&& functor) const
	{
		for (auto&& [className, classData] : m_classes)
			functor(className, classData);
	}
}
