#include "FileTransferWorker.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QThread>


FileTransferWorker::FileTransferWorker(QObject *partent)
    :QObject(partent)
{
}

FileTransferWorker::~FileTransferWorker()
{
}

void FileTransferWorker::startSend(const QString filePath)
{
    if(filePath.isEmpty()){
        emit errorOccurred("[FileTransferWorker] file path is empty");
        return;
    }


    //创建文件对象
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        emit errorOccurred("[FileTransferWorker] " + file.errorString());
        return;
    }
    QFileInfo fileInfo(filePath);
    //生成fileID
    QString fileId=QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

    //构建json文本
    QJsonObject beginObj;
    beginObj["fileName"]=fileInfo.fileName();
    beginObj["fileSize"]=fileInfo.size();
    beginObj["fileId"]=fileId;
    QByteArray beginPyload=QJsonDocument(beginObj).toJson();

    //1.发送文件开始发送消息
    emit fileBeginReady(beginPyload);

    //2.发送数据
    quint64 sentBytes=0;
    constexpr quint64 chunkSize=64 *1024;            //256KB
    while (!file.atEnd())
    {

        //判断是否需要中断,用于client断开时停止文件发送
        if(QThread::currentThread()->isInterruptionRequested()){
            emit errorOccurred("[FileTransferWorker] file transfer canceled");
            qDebug()<<"[FileTransferWorker] canceled";
            return;
        }


        QByteArray chunk=file.read(chunkSize);
        if(chunk.isEmpty()){
            emit errorOccurred("[FileTransferWorker] " + file.errorString());
            return;
        }

        emit fileDataReady(chunk);

        sentBytes+=chunk.size();
        emit progressChanged(sentBytes,fileInfo.size());
        
    }
    


    //3.发送文件结束发送信号
    QJsonObject endObj;
    endObj["fileId"]=fileId;
    QByteArray endPayload=QJsonDocument(endObj).toJson();
    emit fileEndReady(endPayload);
    emit finished();

}
