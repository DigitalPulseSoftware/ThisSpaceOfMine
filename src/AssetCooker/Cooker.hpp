// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_ASSETCOOKER_COOKER_HPP
#define TSOM_ASSETCOOKER_COOKER_HPP

#include <NazaraUtils/FixedVector.hpp>
#include <filesystem>

namespace Nz
{
	class TaskScheduler;
}

namespace tsom
{
	class Cooker
	{
		public:
			using InputFileList = Nz::HybridVector<std::filesystem::path, 1>;
			using OutputFileList = Nz::HybridVector<std::filesystem::path, 1>;

			virtual ~Cooker();

			virtual void Cook(Nz::TaskScheduler& taskScheduler) = 0;

			virtual InputFileList GetInputFiles() const = 0;
			virtual OutputFileList GetOutputFiles() const = 0;
	};
}

#endif // TSOM_ASSETCOOKER_COOKER_HPP
