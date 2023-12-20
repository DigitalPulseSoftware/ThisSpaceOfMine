// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_EXPORT_HPP
#define TSOM_SERVERLIB_EXPORT_HPP

#include <NazaraUtils/Prerequisites.hpp>

#ifdef TSOM_SERVERLIB_STATIC
	#define TSOM_SERVERLIB_API
#else
	#ifdef TSOM_SERVERLIB_BUILD
		#define TSOM_SERVERLIB_API NAZARA_EXPORT
	#else
		#define TSOM_SERVERLIB_API NAZARA_IMPORT
	#endif
#endif

#endif
