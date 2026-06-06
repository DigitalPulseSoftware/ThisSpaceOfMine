// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

// no header guards

#if !defined(TSOM_ENTITYPROPERTYTYPE)
#error You must define TSOM_ENTITYPROPERTYTYPE before including this file
#endif

#ifndef TSOM_ENTITYPROPERTYTYPE_LAST
#define TSOM_ENTITYPROPERTYTYPE_LAST(V, X, UT) TSOM_ENTITYPROPERTYTYPE(V, X, UT)
#endif

TSOM_ENTITYPROPERTYTYPE( 0, Bool, bool)
TSOM_ENTITYPROPERTYTYPE( 1, Float, float)
TSOM_ENTITYPROPERTYTYPE( 2, Float2, Nz::Vector2f)
TSOM_ENTITYPROPERTYTYPE( 3, Float3, Nz::Vector3f)
TSOM_ENTITYPROPERTYTYPE( 4, Float4, Nz::Vector4f)
TSOM_ENTITYPROPERTYTYPE( 5, FloatPosition, Nz::Vector2f)
TSOM_ENTITYPROPERTYTYPE( 6, FloatPosition3D, Nz::Vector3f)
TSOM_ENTITYPROPERTYTYPE( 7, FloatRect, Nz::Rectf)
TSOM_ENTITYPROPERTYTYPE( 8, FloatSize, Nz::Vector2f)
TSOM_ENTITYPROPERTYTYPE( 9, FloatSize3D, Nz::Vector3f)
TSOM_ENTITYPROPERTYTYPE(10, Integer, Nz::Int64)
TSOM_ENTITYPROPERTYTYPE(11, Integer2, Nz::Vector2i64)
TSOM_ENTITYPROPERTYTYPE(12, Integer3, Nz::Vector3i64)
TSOM_ENTITYPROPERTYTYPE(13, Integer4, Nz::Vector4i64)
TSOM_ENTITYPROPERTYTYPE(14, IntegerPosition, Nz::Vector2i64)
TSOM_ENTITYPROPERTYTYPE(15, IntegerPosition3D, Nz::Vector3i64)
TSOM_ENTITYPROPERTYTYPE(16, IntegerRect, Nz::Recti64)
TSOM_ENTITYPROPERTYTYPE(17, IntegerSize, Nz::Vector2i64)
TSOM_ENTITYPROPERTYTYPE(18, IntegerSize3D, Nz::Vector3i64)
TSOM_ENTITYPROPERTYTYPE_LAST(19, String, std::string)

#undef TSOM_ENTITYPROPERTYTYPE
#undef TSOM_ENTITYPROPERTYTYPE_LAST
