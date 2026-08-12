#pragma once

#include "Packet.h"

#include <QByteArray>
#include <optional>

namespace RemoteLink {

class PacketCodec
{
public:
    static QByteArray encode(const Packet &packet);

    void appendData(const QByteArray &data);
    std::optional<Packet> takePacket();
    void clear();

private:
    QByteArray m_buffer;

    bool isValidMessageType(quint16 type);
};

} // namespace RemoteLink
