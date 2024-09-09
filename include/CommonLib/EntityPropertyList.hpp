// Copyright (C) 2024 Jérôme Leclercq
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
TSOM_ENTITYPROPERTYTYPE( 2, FloatPosition, Nz::Vector2f)
TSOM_ENTITYPROPERTYTYPE( 3, FloatPosition3D, Nz::Vector3f)
TSOM_ENTITYPROPERTYTYPE( 4, FloatRect, Nz::Rectf)
TSOM_ENTITYPROPERTYTYPE( 5, FloatSize, Nz::Vector2f)
TSOM_ENTITYPROPERTYTYPE( 6, FloatSize3D, Nz::Vector3f)
TSOM_ENTITYPROPERTYTYPE( 7, Integer, Nz::Int64)
TSOM_ENTITYPROPERTYTYPE( 8, IntegerPosition, Nz::Vector2i64)
TSOM_ENTITYPROPERTYTYPE( 9, IntegerPosition3D, Nz::Vector3i64)
TSOM_ENTITYPROPERTYTYPE(10, IntegerRect, Nz::Recti64)
TSOM_ENTITYPROPERTYTYPE(11, IntegerSize, Nz::Vector2i64)
TSOM_ENTITYPROPERTYTYPE(12, IntegerSize3D, Nz::Vector3i64)
TSOM_ENTITYPROPERTYTYPE_LAST(13, String, std::string)

#undef TSOM_ENTITYPROPERTYTYPE
#undef TSOM_ENTITYPROPERTYTYPE_LAST
