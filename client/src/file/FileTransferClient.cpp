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


    if (!session){
        emit errorOccurred("[FileTransferClient] session is null");
        return;
    }

    if(filePath.isEmpty()){
        emit errorOccurred("[FileTransferClient] file path is null");
        return;
    }

    m_worker=new FileTransferWorker;
    m_workerThread=new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished,m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished,m_workerThread, &QObject::deleteLater);

    connect(m_workerThread,&QThread::finished,this,[this](){
        m_worker=nullptr;
        m_workerThread=nullptr;
    });

    //子线程发来文件开始信息
    connect(m_worker,&FileTransferWorker::fileBeginReady,this,[this,session](const QByteArray &beginPyload){
        if(!session->sendPacket(RemoteLink::MessageType::FileBegin,beginPyload)){
            emit errorOccurred("[FileTransferClient] send FileBegin failed");
        }
    });

    //子线程发来文件数据信息
    connect(m_worker,&FileTransferWorker::fileDataReady,this,[this,session](const QByteArray &dataPyload){
        if(!session->sendPacket(RemoteLink::MessageType::FileData,dataPyload)){
            emit errorOccurred("[FileTransferClient] send FileData failed");
        }
    });

    //子线程发来文件结束信息
    connect(m_worker,&FileTransferWorker::fileEndReady,this,[this,session](const QByteArray &endPyload){
        if(!session->sendPacket(RemoteLink::MessageType::FileEnd,endPyload)){
            emit errorOccurred("[FileTransferClient] send FileEnd failed");
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