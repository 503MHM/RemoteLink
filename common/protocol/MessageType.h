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
    FileBegin = 30,
    FileData = 31,
    FileEnd = 32,
    Heartbeat = 40
};

} // namespace RemoteLink
