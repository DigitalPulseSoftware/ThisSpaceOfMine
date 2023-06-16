// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline Nz::Vector3f Planet::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline float Planet::GetCornerRadius() const
	{
		return m_cornerRadius;
	}

	inline std::size_t Planet::GetGridDimensions() const
	{
		return m_gridDimensions;
	}

	inline float Planet::GetTileSize() const
	{
		return m_tileSize;
	}

	inline void Planet::UpdateCornerRadius(float cornerRadius)
	{
		m_cornerRadius = cornerRadius;
	}
}

