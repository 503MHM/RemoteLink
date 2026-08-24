#include "VideoFramePayload.h"

#include <QDataStream>
#include <QIODevice>

namespace RemoteLink
{

constexpr qsizetype VideoFrameHeaderSize=sizeof(quint32)+sizeof(quint16)+sizeof(quint16)+sizeof(quint64)+sizeof(quint32);

QByteArray encodeVideoFramePayload(const VideoFrameHeader &header, const QByteArray &frameData)
{
    QByteArray payload;
    payload.reserve(VideoFrameHeaderSize+frameData.size());

    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream<<header.frameId
          <<header.codec
          <<header.flags
          <<header.timestampMs
          <<header.dataSize;

    payload.append(frameData);

    return payload;
}

bool decodeVideoFramePayload(const QByteArray &payload, VideoFrameHeader *header, QByteArray *frameData)
{
    if(!header || !frameData)
        return false;    

    if(payload.size()<VideoFrameHeaderSize)
        return false;

    QDataStream stream(payload.left(VideoFrameHeaderSize));
    stream.setByteOrder(QDataStream::BigEndian);
    stream>>header->frameId
          >>header->codec
          >>header->flags
          >>header->timestampMs
          >>header->dataSize;

    *frameData=payload.mid(VideoFrameHeaderSize);
    
    if(header->dataSize!=static_cast<quint32>(frameData->size()))
    {
        return false;
    }
    
    return true;
}

}