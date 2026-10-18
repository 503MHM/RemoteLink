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


QByteArray encodeUdpVideoPacket(const UdpVideoPacketHeader &header, const QByteArray &fragmentData)
{
    if(header.magic!=UdpVideoPacketMagic || 
        header.packetCount == 0 ||
        header.packetCount>MaxUdpVideoPacketCount || 
        header.packetIndex>=header.packetCount || 
        fragmentData.isEmpty() || 
        fragmentData.size()>MaxUdpVideoFragmentPayloadSize)
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

bool decodeUdpVideoPacket(const QByteArray &packet, UdpVideoPacketHeader *header, QByteArray *fragmentData)
{
    if(!header || !fragmentData)
    {
        return false;
    }

    if(packet.size()<UdpVideoPacketHeaderSize)
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

    if (stream.status() != QDataStream::Ok ||
        header->magic != UdpVideoPacketMagic ||
        header->packetCount == 0 ||
        header->packetCount > MaxUdpVideoPacketCount ||
        header->packetIndex >= header->packetCount ||
        header->payloadSize == 0 ||
        header->payloadSize > MaxUdpVideoFragmentPayloadSize ||
        packet.size() != UdpVideoPacketHeaderSize + header->payloadSize) {
        return false;
    }

    *fragmentData=packet.mid(UdpVideoPacketHeaderSize);


    return true;
}
}//namespace RemoteLink
