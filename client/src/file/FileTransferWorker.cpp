#include "FileTransferWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QThread>
#include "protocol/FileTransferPayload.h"

FileTransferWorker::FileTransferWorker(QObject *partent)
    :QObject(partent)
{
}

FileTransferWorker::~FileTransferWorker()
{
}

void FileTransferWorker::readNextChunk()
{
    
    if(QThread::currentThread()->isInterruptionRequested()){
        //已断开连接，停止读取文件
        emit errorOccurred("[FileTransferWorker] file transfer canceled");
        return;
    }

    if(!m_file.isOpen()){
        emit errorOccurred("[FileTransferWorker] file is not open");
        return;
    }

    if(m_file.atEnd()){
        //读取完毕

        QJsonObject endObj;
        endObj["fileId"] = m_fileId;
        emit fileEndReady(QJsonDocument(endObj).toJson());
        emit finished();
        return;
    }
    constexpr qint64 chunkSize=256*1024;
    QByteArray chunk=m_file.read(chunkSize);

    RemoteLink::FileDataHeader payloadHeader;
    payloadHeader.offset=static_cast<quint64>(m_sentBytes);
    payloadHeader.chunkIndex=m_chunkIndex++;
    payloadHeader.dataSize=static_cast<quint32>(chunk.size());

    if(chunk.isEmpty()){
        emit errorOccurred("[FileTransferWorker] " + m_file.errorString());
        return;
    }

    m_sentBytes+=chunk.size();    

    QByteArray payload=RemoteLink::encodeFileDataPayload(payloadHeader,chunk);

    emit fileDataReady(payload);
    emit progressChanged(m_sentBytes,m_fileInfo.size());

}

void FileTransferWorker::startSend(const QString filePath)
{
    if(filePath.isEmpty()){
        emit errorOccurred("[FileTransferWorker] file path is empty");
        return;
    }


    m_file.setFileName(filePath);
    if(!m_file.open(QIODevice::ReadOnly)){
        emit errorOccurred("[FileTransferWorker] " + m_file.errorString());
        return;
    }

    m_fileInfo=QFileInfo(filePath);
    m_fileId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_sentBytes=0;
    m_chunkIndex=0;

    //构建json文本
    QJsonObject beginObj;
    beginObj["fileName"]=m_fileInfo.fileName();
    beginObj["fileSize"]=m_fileInfo.size();
    beginObj["fileId"]=m_fileId;
    QByteArray beginPyload=QJsonDocument(beginObj).toJson();

    //发送文件开始发送消息
    emit fileBeginReady(beginPyload);
}
