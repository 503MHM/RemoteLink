#include "VideoFrameParser.h"
#include "protocol/VideoFramePayload.h"

VideoFrameParser::VideoFrameParser(QObject *parent)
    : QObject(parent)
{
}


void VideoFrameParser::parseFramePayload(const QByteArray &payload)
{
    RemoteLink::VideoFrameHeader videoFrameHeader;
    QByteArray jpegData;

    if (!RemoteLink::decodeVideoFramePayload(payload, &videoFrameHeader, &jpegData))
    {
        return;
    }

    emit jpegFrameReady(jpegData);
}
