// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline float sdRoundBox(const Nz::Vector3f& pos, const Nz::Vector3f& halfDims, float cornerRadius)
	{
		Nz::Vector3f edgeDistance = pos.GetAbs() - halfDims + Nz::Vector3f(cornerRadius);
		float outsideDistance = edgeDistance.Maximize(Nz::Vector3f::Zero()).GetLength();
		float insideDistance = std::min(std::max({ edgeDistance.x, edgeDistance.y, edgeDistance.z }), 0.f);
		return outsideDistance + insideDistance - cornerRadius;
	}
}
