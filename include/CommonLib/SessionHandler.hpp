// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SESSIONHANDLER_HPP
#define TSOM_COMMONLIB_SESSIONHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <array>

namespace Nz
{
	class NetPacket;
}

namespace tsom
{
	class TSOM_COMMONLIB_API SessionHandler
	{
		public:
			using HandlerFunc = void(*)(SessionHandler&, Nz::NetPacket&&);
			using HandlerTable = std::array<HandlerFunc, PacketCount>;

			SessionHandler() = default;
			SessionHandler(const SessionHandler&) = delete;
			SessionHandler(SessionHandler&&) = delete;
			virtual ~SessionHandler();

			void HandlePacket(Nz::NetPacket&& netPacket);

			virtual void OnUnexpectedPacket();

			SessionHandler& operator=(const SessionHandler&) = delete;
			SessionHandler& operator=(SessionHandler&&) = delete;

		protected:
			template<typename T> void SetupHandlerTable();

		private:
			template<typename T> static constexpr HandlerTable BuildHandlerTable();

			const HandlerTable* m_handlerTable;
	};
}

#include <CommonLib/SessionHandler.inl>

#endif
