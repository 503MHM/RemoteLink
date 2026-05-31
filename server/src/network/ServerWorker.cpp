#include "ServerWorker.h"
#include "RemoteServer.h"

ServerWorker::ServerWorker(QObject *partent)
    :QObject(partent)
{
}


ServerWorker::~ServerWorker()
{
    stop();
}


void ServerWorker::start(quint16 port){

    if(m_server){
        return;
    }

    m_server=new RemoteServer(this);

    connect(m_server,&RemoteServer::clientConnected,this,&ServerWorker::clientConnected);
    connect(m_server,&RemoteServer::clientDisconnected,this,&ServerWorker::clientDisconnected);

    connect(m_server,&RemoteServer::receiveStarted,this,&ServerWorker::receiveStarted);
    connect(m_server,&RemoteServer::receiveProgress,this,&ServerWorker::receiveProgress);
    connect(m_server,&RemoteServer::receiveFinished,this,&ServerWorker::receiveFinished);
    connect(m_server,&RemoteServer::receiveError,this,&ServerWorker::receiveError);

    bool ok=m_server->listen(port);

    emit started(ok);

    if(!ok){
        delete m_server;
        m_server=nullptr;
        emit errorOccurred("server listen failed");
    }

}


void ServerWorker::stop()
{

    if(!m_server){
        emit stoped();
        return;
    }

    delete m_server;
    m_server=nullptr;
    emit stoped();
}

void ServerWorker::disconnectClient()
{

    if(!m_server){
        return;
    }

    m_server->disconnectClient();
}
