#include "FileTransferPayload.h"
#include <QIODevice>
#include <QDataStream>

namespace RemoteLink
{
    constexpr qsizetype FileDataHeaderSize=sizeof(quint64)+sizeof(quint32)+sizeof(quint32);

QByteArray RemoteLink::encodeFileDataPayload(const FileDataHeader &header, const QByteArray &chunkData)
{
    QByteArray payload;
    payload.reserve(FileDataHeaderSize+chunkData.size());

    QDataStream stream(&payload,QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream<<header.offset
          <<header.chunkIndex
          <<header.dataSize;
    
    payload.append(chunkData);

    return payload;
}

bool RemoteLink::decodeFileDataPayload(const QByteArray &payload, FileDataHeader *header, QByteArray *chunkData)
{
    if(payload.size()<FileDataHeaderSize)
    {
        return false;
    }

    if(!header || !chunkData)
    {
        return false;
    }

    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream>>header->offset
          >>header->chunkIndex
          >>header->dataSize;

    *chunkData=payload.mid(FileDataHeaderSize);

    if(header->dataSize!=static_cast<quint32>(chunkData->size()))
    {
        return false;
    }

    return true;
}

}