// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_EXPORT_HPP
#define TSOM_CLIENTLIB_EXPORT_HPP

#include <NazaraUtils/Prerequisites.hpp>

#ifdef TSOM_CLIENTLIB_STATIC
	#define TSOM_CLIENTLIB_API
#else
	#ifdef TSOM_CLIENTLIB_BUILD
		#define TSOM_CLIENTLIB_API NAZARA_EXPORT
	#else
		#define TSOM_CLIENTLIB_API NAZARA_IMPORT
	#endif
#endif

#endif // TSOM_CLIENTLIB_EXPORT_HPP
