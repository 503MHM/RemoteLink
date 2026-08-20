#pragma once

#include <QtGlobal>
#include <QByteArray>

namespace RemoteLink{

    struct FileDataHeader
    {
        quint64 offset=0;             //当前分片在文件中的起始偏移量
        quint32 chunkIndex=0;         //分片序号，从0开始
        quint32 dataSize=0;           //分片数据长度
    };
    
    QByteArray encodeFileDataPayload(const FileDataHeader &header,const QByteArray &chunkData);

    bool decodeFileDataPayload(const QByteArray &payload, FileDataHeader *header,QByteArray *chunkData);

}