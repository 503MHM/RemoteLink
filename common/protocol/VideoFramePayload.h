#pragma once

#include <QByteArray>
#include <QtGlobal>

namespace RemoteLink
{

enum class VideoCodec : quint16
{
    Jpeg=1
};

struct VideoFrameHeader
{
    quint32 frameId=0;
    quint16 codec=static_cast<quint16>(VideoCodec::Jpeg);
    quint16 flags=0;
    quint64 timestampMs=0;
    quint32 dataSize=0;
};

QByteArray encodeVideoFramePayload(
    const VideoFrameHeader &header,
    const QByteArray &frameData);

bool decodeVideoFramePayload(const QByteArray &payload,
                             VideoFrameHeader *header,
                             QByteArray *frameData);

}
