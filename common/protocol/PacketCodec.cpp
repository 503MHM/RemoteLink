#include "PacketCodec.h"
#include "NetworkConstants.h"

#include <QDataStream>
#include <QIODevice>

namespace RemoteLink {

namespace {
constexpr qsizetype HeaderSize = sizeof(quint32) + sizeof(quint16) + sizeof(quint32) + sizeof(quint32) + sizeof(quint32);
}

QByteArray PacketCodec::encode(const Packet &packet)
{
    if (packet.payload.size() > static_cast<qsizetype>(MaxPacketPayloadSize)) {
        return {};
    }

    QByteArray data;
    data.reserve(static_cast<int>(HeaderSize + packet.payload.size()));

    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << PacketMagic;
    stream << static_cast<quint16>(packet.type);
    stream << packet.sessionId;
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
        // 先从缓冲区头部解析固定长度包头。
        QDataStream headerStream(m_buffer.left(HeaderSize));
        headerStream.setByteOrder(QDataStream::BigEndian);

        quint32 magic = 0;
        quint16 type = 0;
        quint32 length = 0;
        quint32 sequence = 0;
        quint32 sessionId = 0;
        headerStream >> magic >> type >> sessionId >> length >> sequence;

        if (magic != PacketMagic) {
            // 魔数不匹配时丢弃一个字节，尝试重新对齐到下一个包头。
            m_buffer.remove(0, 1);
            continue;
        }

        if(length>MaxPacketPayloadSize)
        {
            //载荷大于最大长度
            m_buffer.remove(0,1);
            continue;
        }

        if(!isValidMessageType(type))
        {
            //类型不是已有的
            m_buffer.remove(0,1);
            continue;
        }

        const qsizetype packetSize = HeaderSize + static_cast<qsizetype>(length);
        if (m_buffer.size() < packetSize) {
            // 数据还没收完整，等待下一次 appendData() 后继续解析。
            return std::nullopt;
        }

        // 取出完整 payload，并从缓冲区移除已经消费的一个完整包。
        Packet packet;
        packet.type = static_cast<MessageType>(type);
        packet.sessionId = sessionId;
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

bool PacketCodec::isValidMessageType(quint16 type)
{
    switch (static_cast<MessageType>(type)) {
        case MessageType::Discovery:
        case MessageType::DiscoveryReply:
        case MessageType::Heartbeat:
        case MessageType::HeartbeatAck:
        case MessageType::FrameData:
        case MessageType::MouseMove:
        case MessageType::MouseClick:
        case MessageType::MouseWheel:
        case MessageType::MousePress:
        case MessageType::MouseRelease:
        case MessageType::KeyEvent:
        case MessageType::FileBegin:
        case MessageType::FileData:
        case MessageType::FileEnd:
        case MessageType::ServerKick:
            return true;
        }
    
        return false;
}

} // namespace RemoteLink
