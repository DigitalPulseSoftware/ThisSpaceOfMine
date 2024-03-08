// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_AVERAGEVALUES_HPP
#define TSOM_COMMONLIB_UTILITY_AVERAGEVALUES_HPP

#include <vector>

namespace tsom
{
	template<typename T>
	class AverageValues
	{
		public:
			explicit AverageValues(std::size_t maxValueCount);

			T GetAverageValue() const;

			void InsertValue(T value);

		private:
			std::vector<T> m_values;
			std::size_t m_maxValueCount;
			T m_valueSum;
	};
}

#include <CommonLib/Utility/AverageValues.inl>

#endif // TSOM_COMMONLIB_UTILITY_AVERAGEVALUES_HPP
