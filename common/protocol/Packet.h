#pragma once

#include "MessageType.h"

#include <QByteArray>
#include <QtGlobal>

namespace RemoteLink {

constexpr quint32 PacketMagic = 0x524C4B31; // RLK1

struct PacketHeader
{
    quint32 magic = PacketMagic;
    quint16 type = 0;
    quint32 sessionId = 0;
    quint32 length = 0;
    quint32 sequence = 0;
};

struct Packet
{
    MessageType type = MessageType::Heartbeat;
    quint32 sessionId = 0;
    quint32 sequence = 0;
    QByteArray payload;
};

} // namespace RemoteLink
