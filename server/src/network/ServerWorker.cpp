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
