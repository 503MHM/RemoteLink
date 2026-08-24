#pragma once

#include <QByteArray>
#include <QtGlobal>

class VideoSender
{
public:
    //VideoSender(/* args */);
    
    QByteArray bulidJpegFramePayload(const QByteArray &jpegData);

    void reset();

private:
    quint32 m_nextFrameId=1;
};
