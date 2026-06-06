// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename F>
	Nz::Vector3f SignedDistanceNormal(const Nz::Vector3f& pos, F&& distCallback)
	{
		constexpr float epsilon = 0.0001f;
		return Nz::Vector3f::Normalize(
			Nz::Vector3f( 1.0f, -1.0f, -1.0f) * distCallback(pos + Nz::Vector3f( 1.0f, -1.0f, -1.0f) * epsilon) +
			Nz::Vector3f(-1.0f, -1.0f,  1.0f) * distCallback(pos + Nz::Vector3f(-1.0f, -1.0f,  1.0f) * epsilon) +
			Nz::Vector3f(-1.0f,  1.0f, -1.0f) * distCallback(pos + Nz::Vector3f(-1.0f,  1.0f, -1.0f) * epsilon) +
			Nz::Vector3f( 1.0f,  1.0f,  1.0f) * distCallback(pos + Nz::Vector3f( 1.0f,  1.0f,  1.0f) * epsilon)
		);
	}

	inline float sdRoundBox(const Nz::Vector3f& pos, const Nz::Vector3f& halfDims, float cornerRadius)
	{
		Nz::Vector3f edgeDistance = pos.GetAbs() - halfDims + Nz::Vector3f(cornerRadius);
		float outsideDistance = edgeDistance.Maximize(Nz::Vector3f::Zero()).GetLength();
		float insideDistance = std::min(std::max({ edgeDistance.x, edgeDistance.y, edgeDistance.z }), 0.f);
		return outsideDistance + insideDistance - cornerRadius;
	}

	inline float sdTorus(const Nz::Vector3f& pos, const Nz::Vector2f& dims)
	{
		Nz::Vector2f q(Nz::Vector2f(pos.x, pos.z).GetLength() - dims.x, pos.y);
		return q.GetLength() - dims.y;
	}
}
