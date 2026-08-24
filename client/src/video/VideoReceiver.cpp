#include "VideoReceiver.h"
#include "protocol/VideoFramePayload.h"

VideoReceiver::VideoReceiver(QObject * parent)
    :QObject(parent)
{
}


void VideoReceiver::onFrameDataReceived(const QByteArray &payload)
{
    RemoteLink::VideoFrameHeader videoFrameHeader;
    QByteArray jpegData;

    if(!RemoteLink::decodeVideoFramePayload(payload,&videoFrameHeader,&jpegData))
    {
        return;
    }

    emit jpegDataReceived(jpegData);
}