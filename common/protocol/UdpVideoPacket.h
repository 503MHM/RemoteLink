#pragma once

#include <QByteArray>
#include <QtGlobal>

namespace RemoteLink
{

inline constexpr qsizetype MaxUdpVideoFragmentPayloadSize=1200;
inline constexpr quint16 MaxUdpVideoPacketCount=4096;
inline constexpr qsizetype MaxUdpVideoFramePayloadSize=4*1024*1024;

constexpr quint32 UdpVideoPacketMagic=0x524c5631;

struct UdpVideoPacketHeader
{
    quint32 magic=UdpVideoPacketMagic;
    quint32 sessionId=0;
    quint32 frameId=0;
    quint16 packetIndex=0;
    quint16 packetCount=0;
    quint16 payloadSize=0;
    quint16 flags=0;
};

QByteArray encodeUdpVideoPacket(const UdpVideoPacketHeader &header,const QByteArray &fragmentData);

bool decodeUdpVideoPacket(const QByteArray &packet,UdpVideoPacketHeader *header,QByteArray *fragmentData);


}