// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_NETWORKSTRINGSTORE_HPP
#define TSOM_COMMONLIB_PROTOCOL_NETWORKSTRINGSTORE_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Protocol/SecuredString.hpp>
#include <tsl/hopscotch_map.h>
#include <limits>
#include <optional>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API NetworkStringStore
	{
		public:
			inline NetworkStringStore();
			~NetworkStringStore() = default;

			Packets::NetworkStrings BuildPacket(Nz::UInt32 firstId = 0) const;

			inline void Clear();

			inline Nz::UInt32 CheckStringIndex(const std::string& string) const;

			void FillStore(Nz::UInt32 firstId, std::vector<SecuredString<1024>> strings);

			inline const std::string& GetString(Nz::UInt32 id) const;
			inline Nz::UInt32 GetStringIndex(const std::string& string) const;

			inline Nz::UInt32 RegisterString(std::string string);

			static constexpr Nz::UInt32 InvalidIndex = std::numeric_limits<Nz::UInt32>::max();

		private:
			tsl::hopscotch_map<std::string, Nz::UInt32> m_stringMap;
			std::vector<std::string> m_strings;
	};
}

#include <CommonLib/Protocol/NetworkStringStore.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_NETWORKSTRINGSTORE_HPP
