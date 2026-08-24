#include "VideoSender.h"
#include "protocol/VideoFramePayload.h"
#include <QDateTime>

QByteArray VideoSender::bulidJpegFramePayload(const QByteArray &jpegData)
{
    if(jpegData.isEmpty())
        return {};

    RemoteLink::VideoFrameHeader videoFrameHeader;
    videoFrameHeader.frameId=m_nextFrameId;
    videoFrameHeader.codec=static_cast<quint16>(RemoteLink::VideoCodec::Jpeg);
    videoFrameHeader.flags=0;
    videoFrameHeader.dataSize=static_cast<quint32>(jpegData.size());
    videoFrameHeader.timestampMs=static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());

    const QByteArray payload=RemoteLink::encodeVideoFramePayload(videoFrameHeader,jpegData);
    if(payload.isEmpty())
        return {};

    m_nextFrameId++;
    return payload;
}

void VideoSender::reset()
{
    m_nextFrameId=1;
}
