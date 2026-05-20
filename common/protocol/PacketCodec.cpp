#include "PacketCodec.h"

#include <QDataStream>
#include <QIODevice>

namespace RemoteLink {

namespace {
constexpr qsizetype HeaderSize = sizeof(quint32) + sizeof(quint16) + sizeof(quint32) + sizeof(quint32);
}

QByteArray PacketCodec::encode(const Packet &packet)
{
    QByteArray data;
    data.reserve(static_cast<int>(HeaderSize + packet.payload.size()));

    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << PacketMagic;
    stream << static_cast<quint16>(packet.type);
    stream << static_cast<quint32>(packet.payload.size());
    stream << packet.sequence;
    data.append(packet.payload);

    return data;
}

void PacketCodec::appendData(const QByteArray &data)
{
    m_buffer.append(data);
}

std::optional<Packet> PacketCodec::takePacket()
{
    while (m_buffer.size() >= HeaderSize) {
        QDataStream headerStream(m_buffer.left(HeaderSize));
        headerStream.setByteOrder(QDataStream::BigEndian);

        quint32 magic = 0;
        quint16 type = 0;
        quint32 length = 0;
        quint32 sequence = 0;
        headerStream >> magic >> type >> length >> sequence;

        if (magic != PacketMagic) {
            m_buffer.remove(0, 1);
            continue;
        }

        const qsizetype packetSize = HeaderSize + static_cast<qsizetype>(length);
        if (m_buffer.size() < packetSize) {
            return std::nullopt;
        }

        Packet packet;
        packet.type = static_cast<MessageType>(type);
        packet.sequence = sequence;
        packet.payload = m_buffer.mid(HeaderSize, length);
        m_buffer.remove(0, packetSize);

        return packet;
    }

    return std::nullopt;
}

void PacketCodec::clear()
{
    m_buffer.clear();
}

} // namespace RemoteLink
