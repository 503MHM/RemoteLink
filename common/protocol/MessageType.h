#pragma once

#include <QtGlobal>

namespace RemoteLink {

enum class MessageType : quint16
{
    Discovery = 101,
    DiscoveryReply = 102,

    FrameData = 301,
    
    MouseMove = 401,
    MouseClick = 402,
    MouseWheel = 403,
    MousePress = 404,
    MouseRelease = 405,
    KeyEvent = 406,
    
    FileBegin = 501,
    FileData = 502,
    FileEnd = 503,
    
    Heartbeat = 201,
    HeartbeatAck = 202,
    
    ServerKick = 601
};

} // namespace RemoteLink
