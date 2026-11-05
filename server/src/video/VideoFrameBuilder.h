#pragma once

#include <QByteArray>
#include <QtGlobal>

struct EncodedVideoFrame
{
    quint32 frameId = 0;
    QByteArray payload;
};

class VideoFrameBuilder
{
public:
    EncodedVideoFrame buildJpegFramePayload(const QByteArray &jpegData);

    void reset();

private:
    quint32 m_nextFrameId = 1;
};
