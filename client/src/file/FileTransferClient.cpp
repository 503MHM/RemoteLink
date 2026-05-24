#include "FileTransferClient.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUuid>


FileTransferClient::FileTransferClient(QObject *partent)
    :QObject(partent)
{

}

FileTransferClient::~FileTransferClient()
{
}

void FileTransferClient::sendFile(const QString &filePath, RemoteSession *session)
{
    if(!session){
        emit errorOccurred("[FileTransferClient] session is null");
        return;
    }

    if(filePath.isEmpty()){
        emit errorOccurred("[FileTransferClient] file path is null");
        return;
    }

    //创建文件对象
    QFile file(filePath);
    //打开文件
    if(!file.open(QIODevice::ReadOnly)){
        emit errorOccurred("[FileTransferClient] "+file.errorString());
        return;
    }
    QFileInfo fileInfo(filePath);
    
    //生成fileID
    QString fileId=QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

    //1.构造json,发送MessageType::FileBegin
    QJsonObject beginobj;
    beginobj["fileName"]=fileInfo.fileName();
    beginobj["fileSize"]=fileInfo.size();
    beginobj["fileId"]=fileId;
    QByteArray beginpayload=QJsonDocument(beginobj).toJson();

    session->sendPacket(RemoteLink::MessageType::FileBegin,beginpayload);

    //2.发送文件数据
    quint64 sentBytes=0;
    constexpr quint64 chunkSize=256 *1024;            //256KB
    while(!file.atEnd()){
        QByteArray chunkData=file.read(chunkSize);

        if(chunkData.isEmpty()){
            return;
        }

        session->sendPacket(RemoteLink::MessageType::FileData,chunkData);

        sentBytes+=chunkData.size();
        emit progressChanged(sentBytes,fileInfo.size());
    }

    //3.发送fileEnd
    QJsonObject endObj;
    endObj["fileId"]=fileId;
    QByteArray endPayload=QJsonDocument(endObj).toJson();
    session->sendPacket(RemoteLink::MessageType::FileEnd,endPayload);

    emit finished();
}