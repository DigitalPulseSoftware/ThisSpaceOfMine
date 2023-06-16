// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/Packets.hpp>

namespace tsom::Packets
{
	void Serialize(PacketSerializer& serializer, NetworkStrings& data)
	{
		serializer &= data.startId;

		serializer.SerializeArraySize(data.strings);
		for (auto& string : data.strings)
			serializer &= string;
	}

	void Serialize(PacketSerializer& serializer, Test& data)
	{
		serializer &= data.str;
	}
}
