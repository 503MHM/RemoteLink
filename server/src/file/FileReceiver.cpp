#include "FileReceiver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QApplication>
#include "protocol/FileTransferPayload.h"

FileReceiver::FileReceiver(QObject *partent)
    :QObject(partent)
{
}

bool FileReceiver::handleFileBegin(const QByteArray &payload)
{
    //解析json
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        qWarning()<<"[file] invalid FileBegin payload";
        return false;
    }
    QJsonObject obj=doc.object();

    QString fileName=obj.value("fileName").toString();
    quint64 fileSize=static_cast<quint64>(obj.value("fileSize").toDouble());
    QString fileId=obj.value("fileId").toString();
    if(fileName.isEmpty() || fileId.isEmpty()){
        qWarning() << "[file] invalid FileBegin fields";
        return false;
    }

    if(m_file.isOpen()) {
        m_file.close();
    }

    //创建目录对象
    QDir dir(QCoreApplication::applicationDirPath()+"/downloads");
    if(!dir.exists()){
        dir.mkdir(".");         //创建目录
    }

    QString savePath=dir.filePath(fileName);
    m_file.setFileName(savePath);
    if(!m_file.open(QIODevice::WriteOnly)){
        qWarning() << "[file] open failed:" << m_file.errorString();
        return false;
    }

    m_fileId=fileId;
    m_totalBytes=fileSize;
    m_receivedBytes=0;
    m_expectedChunkIndex=0;

    //发送给UI文件信息
    emit receiveStarted(fileName,m_totalBytes);

    qDebug() << "[file] begin:" << savePath << "size =" << m_totalBytes;
    return true;
}

bool FileReceiver::handleFileData(const QByteArray &payload)
{
    //判断文件是否已经打开
    if(!m_file.isOpen()){
        qWarning() << "[file] data received but file is not open";
        return false;
    }
    RemoteLink::FileDataHeader fileDataHeader;
    QByteArray chunk;

    if(!RemoteLink::decodeFileDataPayload(payload,&fileDataHeader,&chunk))
    {
        return false;
    }

    if(m_receivedBytes!=fileDataHeader.offset)
    {
        return false;
    }
    if(fileDataHeader.chunkIndex!=m_expectedChunkIndex)
    {
        return false;
    }

    //写入内容
    if(m_file.write(chunk) != fileDataHeader.dataSize){
        qWarning() << "[file] write failed:" << m_file.errorString();
        return false;
    }

    m_receivedBytes+=fileDataHeader.dataSize;
    ++m_expectedChunkIndex;

    //发送给UI文件信息
    emit receiveProgress(m_receivedBytes,m_totalBytes);

    qDebug() << "[file] progress:" << m_receivedBytes << "/" << m_totalBytes;
    return true;
}

bool FileReceiver::handleFileEnd(const QByteArray &payload)
{
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        qWarning() << "[file] invalid FileEnd payload";
        return false;
    }
    QJsonObject obj=doc.object();
    QString fileId=obj.value("fileId").toString();

    if(fileId!=m_fileId){
        qWarning() << "[file] fileId mismatch";
        return false;
    }

    //关闭文件
    if (m_file.isOpen()) {
        m_file.close();
    }

    bool ok = (m_receivedBytes == m_totalBytes);

    //发送给UI文件信息
    emit receiveFinished(ok,QFileInfo(m_file).absoluteFilePath());


    qDebug() << "[file] end, received =" << m_receivedBytes
             << "total =" << m_totalBytes
             << "ok =" << ok;

    m_fileId.clear();
    m_totalBytes = 0;
    m_receivedBytes = 0;

    return ok;
}
