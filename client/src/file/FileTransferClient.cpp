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
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void FileTransferClient::sendFile(const QString &filePath, RemoteSession *session)
{
    //防止重复发送，防止m_workerThread || m_worker被覆盖
    if (m_workerThread || m_worker) {
        emit errorOccurred("[FileTransferClient] file transfer already running");
        return;
    }

    if (!session){
        emit errorOccurred("[FileTransferClient] session is null");
        return;
    }

    if(filePath.isEmpty()){
        emit errorOccurred("[FileTransferClient] file path is null");
        return;
    }

    m_cancelRequested=false;

    m_worker=new FileTransferWorker;
    m_workerThread=new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished,m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished,m_workerThread, &QObject::deleteLater);

    connect(m_workerThread,&QThread::finished,this,[this](){
        m_worker=nullptr;
        m_workerThread=nullptr;
        m_cancelRequested=false;
    });


    constexpr qint64 HightWatermark=2*1024*1024;
    constexpr qint64 LowWatermark=512*1024;

    //请求读取下一个数据块
    auto requestNextChunk=[this,session,HightWatermark](){
        if(m_cancelRequested){
            return;
        }
        if(!m_worker){
            return;
        }
        if(session->bytesToWrite()>HightWatermark){
            //缓冲区的数据大于指定的数值就不读取文件内容
            return;
        }

        //没超过指定值继续读取文件内容到缓冲区
        QMetaObject::invokeMethod(m_worker,"readNextChunk",Qt::QueuedConnection);
    };

    //监听 socket 排水
    connect(session,
            &RemoteSession::writeBufferBeLowWatermark,
            m_worker,
            &FileTransferWorker::readNextChunk,
            Qt::QueuedConnection);


    //子线程发来文件开始信息
    connect(m_worker,&FileTransferWorker::fileBeginReady,this,[this,session,requestNextChunk](const QByteArray &beginPyload){
        if(!session->sendPacket(RemoteLink::MessageType::FileBegin,beginPyload)){

            if(m_cancelRequested) return;

            emit errorOccurred("[FileTransferClient] send FileBegin failed");
            cancel();
        }

        requestNextChunk();

    });

    //子线程发来文件数据信息
    connect(m_worker,&FileTransferWorker::fileDataReady,this,[this,session,requestNextChunk](const QByteArray &dataPyload){
        
        if(m_cancelRequested) return;
        
        if(!session->sendPacket(RemoteLink::MessageType::FileData,dataPyload)){
            emit errorOccurred("[FileTransferClient] send FileData failed");
            cancel();
        }
        qDebug() << "[file] bytesToWrite =" << session->bytesToWrite();
        requestNextChunk();
    });

    //子线程发来文件结束信息
    connect(m_worker,&FileTransferWorker::fileEndReady,this,[this,session](const QByteArray &endPyload){
        
        if(m_cancelRequested) return;
        
        if(!session->sendPacket(RemoteLink::MessageType::FileEnd,endPyload)){
            emit errorOccurred("[FileTransferClient] send FileEnd failed");
            cancel();
        }
    });


    connect(m_worker,&FileTransferWorker::progressChanged,this,&FileTransferClient::progressChanged);

    connect(m_worker,&FileTransferWorker::errorOccurred,this,&FileTransferClient::errorOccurred);

    //文件发送失败退出线程
    connect(m_worker,&FileTransferWorker::errorOccurred,m_workerThread,QThread::quit);

    connect(m_worker,&FileTransferWorker::finished,this,&FileTransferClient::finished);

    //文件发送完毕退出线程
    connect(m_worker,&FileTransferWorker::finished,m_workerThread,QThread::quit);

    connect(m_workerThread,&QThread::started,m_worker,[this,filePath]{
        m_worker->startSend(filePath);
    });


    m_workerThread->start();
}

void FileTransferClient::cancel()
{

    if(m_cancelRequested){
        return;
    }
    m_cancelRequested=true;

    //将中断请求标志设置为true
    if(m_workerThread){
        m_workerThread->requestInterruption();
        qDebug()<<"[FileTransferClient] cancel requested";
    }
}
