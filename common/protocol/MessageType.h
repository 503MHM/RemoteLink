#pragma once

#include <QtGlobal>

namespace RemoteLink {

enum class MessageType : quint16
{
    Discovery = 1,
    DiscoveryReply = 2,
    FrameData = 10,
    MouseMove = 20,
    MouseClick = 21,
    MouseWheel = 22,
    MousePress = 23,
    MouseRelease = 24,
    FileBegin = 30,
    FileData = 31,
    FileEnd = 32,
    Heartbeat = 40,
    HeartbeatAck = 41,
    KeyEvent = 50,
    ServerKick=60
};

} // namespace RemoteLink
