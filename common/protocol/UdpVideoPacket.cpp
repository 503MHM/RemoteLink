#include "UdpVideoPacket.h"
#include <QDataStream>
#include <QIODevice>

namespace RemoteLink{
namespace {
    constexpr qsizetype UdpVideoPacketHeaderSize=
    sizeof(quint32)+sizeof(quint32)+sizeof(quint32)+
    sizeof(quint16)+sizeof(quint16)+sizeof(quint16)+
    sizeof(quint16);
}


QByteArray RemoteLink::encodeUdpVideoPacket(const UdpVideoPacketHeader &header, const QByteArray &fragmentData)
{
    if(fragmentData.size()>std::numeric_limits<quint16>::max())
    {
        return {};
    }
    QByteArray packet;
    packet.reserve(UdpVideoPacketHeaderSize+fragmentData.size());

    QDataStream stream(&packet,QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream<<header.magic
          <<header.sessionId
          <<header.frameId
          <<header.packetIndex
          <<header.packetCount
          <<static_cast<quint16>(fragmentData.size())
          <<header.flags;

    packet.append(fragmentData);

    return packet;
}

bool RemoteLink::decodeUdpVideoPacket(const QByteArray &packet, UdpVideoPacketHeader *header, QByteArray *fragmentData)
{
    if(!header || !fragmentData)
    {
        return false;
    }

    if(packet<UdpVideoPacketHeaderSize)
    {
        return false;
    }

    QDataStream stream(packet.left(UdpVideoPacketHeaderSize));
    stream.setByteOrder(QDataStream::BigEndian);

    stream>>header->magic
          >>header->sessionId
          >>header->frameId
          >>header->packetIndex
          >>header->packetCount
          >>header->payloadSize
          >>header->flags;

    if(header->magic!=UdpVideoPacketMagic)
    {
        return false;
    }

    if(packet.size()!=UdpVideoPacketHeaderSize+header->payloadSize)
    {
        return false;
    }

    *fragmentData=packet.mid(UdpVideoPacketHeaderSize);


    return true;
}
}//namespace RemoteLink
