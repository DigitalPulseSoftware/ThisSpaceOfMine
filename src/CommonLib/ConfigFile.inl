// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <fmt/format.h>
#include <cassert>
#include <limits>
#include <stdexcept>

namespace tsom
{
	inline bool ConfigFile::GetBoolValue(BoolOptionName optionName) const
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<BoolOption>(m_options[optionIndex].data).value;
	}

	template<typename T>
	T ConfigFile::GetFloatValue(FloatOptionName optionName) const
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return static_cast<T>(std::get<FloatOption>(m_options[optionIndex].data).value);
	}

	template<typename T>
	T ConfigFile::GetIntegerValue(IntegerOptionName optionName) const
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);

		long long value = std::get<IntegerOption>(m_options[optionIndex].data).value;
		if constexpr (std::is_unsigned_v<T>)
		{
			if (value < 0)
				throw std::range_error(fmt::format("{} value is smaller than T minimal representable value ({} < 0)", optionName.name, value));

			unsigned long long unsignedValue = static_cast<unsigned long long>(value);
			if (unsignedValue > std::numeric_limits<T>::max())
				throw std::range_error(fmt::format("{} value is greater than T maximal representable value ({} > {})", optionName.name, unsignedValue, std::numeric_limits<T>::max()));

			return static_cast<T>(unsignedValue);
		}
		else
		{
			if (value < std::numeric_limits<T>::min())
				throw std::range_error(fmt::format("{} value is smaller than T minimal representable value ({} < {})", optionName.name, value, std::numeric_limits<T>::min()));

			if (value > std::numeric_limits<T>::max())
				throw std::range_error(fmt::format("{} value is greater than T maximal representable value ({} > {})", optionName.name, value, std::numeric_limits<T>::max()));

			return static_cast<T>(value);
		}
	}

	template<typename T>
	void ConfigFile::RegisterOption(std::string optionName, T&& optionData)
	{
		if (optionData.defaultValue.has_value())
			optionData.value = optionData.defaultValue.value();

		RegisterConfig(std::move(optionName), std::move(optionData));
	}

	inline std::size_t ConfigFile::GetOptionIndex(std::string_view optionName) const
	{
		auto it = m_optionByName.find(optionName);
		assert(it != m_optionByName.end());

		return it->second;
	}

	inline const std::string& ConfigFile::GetStringValue(StringOptionName optionName) const
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<StringOption>(m_options[optionIndex].data).value;
	}

	inline Nz::Signal<bool>& ConfigFile::GetBoolUpdateSignal(BoolOptionName optionName)
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<BoolOption>(m_options[optionIndex].data).OnValueUpdate;
	}

	inline Nz::Signal<double>& ConfigFile::GetFloatUpdateSignal(FloatOptionName optionName)
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<FloatOption>(m_options[optionIndex].data).OnValueUpdate;
	}

	inline Nz::Signal<long long>& ConfigFile::GetIntegerUpdateSignal(IntegerOptionName optionName)
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<IntegerOption>(m_options[optionIndex].data).OnValueUpdate;
	}

	inline Nz::Signal<const std::string&>& ConfigFile::GetStringUpdateSignal(StringOptionName optionName)
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);
		return std::get<StringOption>(m_options[optionIndex].data).OnValueUpdate;
	}

	inline void ConfigFile::RegisterBoolOption(BoolOptionName optionName, std::optional<bool> defaultValue)
	{
		BoolOption boolOption;
		boolOption.defaultValue = std::move(defaultValue);

		RegisterOption(std::string(optionName.name), std::move(boolOption));
	}

	inline void ConfigFile::RegisterFloatOption(FloatOptionName optionName, std::optional<double> defaultValue, FloatValidation validation)
	{
		RegisterFloatOption(optionName, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::move(defaultValue), std::move(validation));
	}

	inline void ConfigFile::RegisterFloatOption(FloatOptionName optionName, double minBounds, double maxBounds, std::optional<double> defaultValue, FloatValidation validation)
	{
		FloatOption floatOption;
		floatOption.defaultValue = std::move(defaultValue);
		floatOption.maxBounds = maxBounds;
		floatOption.minBounds = minBounds;
		floatOption.validation = std::move(validation);

		RegisterOption(std::string(optionName.name), std::move(floatOption));
	}

	inline void ConfigFile::RegisterIntegerOption(IntegerOptionName optionName, std::optional<long long> defaultValue, IntegerValidation validation)
	{
		RegisterIntegerOption(std::move(optionName), std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max(), std::move(defaultValue), std::move(validation));
	}

	inline void ConfigFile::RegisterIntegerOption(IntegerOptionName optionName, long long minBounds, long long maxBounds, std::optional<long long> defaultValue, IntegerValidation validation)
	{
		IntegerOption intOption;
		intOption.defaultValue = std::move(defaultValue);
		intOption.maxBounds = maxBounds;
		intOption.minBounds = minBounds;
		intOption.validation = std::move(validation);

		RegisterOption(std::string(optionName.name), std::move(intOption));
	}

	inline void ConfigFile::RegisterStringOption(StringOptionName optionName, std::optional<std::string> defaultValue, StringValidation validation)
	{
		StringOption strOption;
		strOption.defaultValue = std::move(defaultValue);
		strOption.validation = std::move(validation);

		RegisterOption(std::string(optionName.name), std::move(strOption));
	}

	inline bool ConfigFile::SetBoolValue(BoolOptionName optionName, bool value)
	{
		std::size_t optionIndex = GetOptionIndex(optionName.name);

		BoolOption& option = std::get<BoolOption>(m_options[optionIndex].data);
		if (option.value != value)
		{
			option.OnValueUpdate(value);
			option.value = value;
		}

		return true;
	}
}
