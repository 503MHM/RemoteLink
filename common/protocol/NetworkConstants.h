#pragma once

#include <QtGlobal>

namespace RemoteLink
{

constexpr quint16 TcpPort=45454;
constexpr quint16 DiscoveryPort=45455;

constexpr const char *DiscoveryType="DISCOVERY";
constexpr const char *DiscovertReplyType="DISCOVERY_REPLY";

constexpr quint32 MaxPacketPayloadSize=4*1024*1024;     //4MB


enum  class ProtocolErrorCode : quint8
{
    OK=0,
    InvalidMagic=1,
    UnknownMessageType=2,
    PayLoadTooLarge=3,
    InvalidSession=4,
    Unauthorized=5,
    Busy=6,
    InternalError
};

}//namespace RemoteLink