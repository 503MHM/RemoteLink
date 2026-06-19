#include "RemoteServer.h"
#include "../capture/ScreenCapture.h"
#include "protocol/PacketCodec.h"
#include "./control/MouseController.h"
#include "control/KeyboardController.h"
#include "file/FileReceiver.h"

#include <QDebug>

using namespace RemoteLink;

RemoteServer::RemoteServer(QObject *parent)
    : QObject(parent)
    ,m_frameSequence(0)
    ,m_currClientSocket(nullptr)
    {
    connect(&m_server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection);
    

    //创建截图子线程
    m_captureThread=new QThread(this);
    m_captureWorker=new CaptureWorker;
    m_captureWorker->moveToThread(m_captureThread);
    m_captureThread->start();

    connect(this,&RemoteServer::startCaptureRequested,m_captureWorker,&CaptureWorker::startCaptureWork);
    connect(this,&RemoteServer::stopCaptureRequested,m_captureWorker,&CaptureWorker::stopCaptureWork);

    //收到子线程的信号后发送截图
    connect(m_captureWorker,&CaptureWorker::frameReady,this,[=](const QByteArray &frameData){
        
        //打包
        Packet packet;
        packet.type=MessageType::FrameData;
        packet.sequence=m_frameSequence++;
        packet.payload=frameData;
        
        QByteArray sendData=PacketCodec::encode(packet);

        if(!m_currClientSocket || m_currClientSocket->state()!=QAbstractSocket::SocketState::ConnectedState){
            qDebug()<<"[RemoteServer] send frame data filed!,disconnected";
            return;
        }

        if (m_currClientSocket->bytesToWrite() > 2 * 1024 * 1024) {
            return;
        }

        m_currClientSocket->write(sendData);
    });


    connect(m_captureThread, &QThread::finished,m_captureWorker, &QObject::deleteLater);

    //连接文件传输相关信号
    connect(&m_fileReceiver,&FileReceiver::receiveStarted,this,&RemoteServer::receiveStarted);
    connect(&m_fileReceiver,&FileReceiver::receiveProgress,this,&RemoteServer::receiveProgress);
    connect(&m_fileReceiver,&FileReceiver::receiveFinished,this,&RemoteServer::receiveFinished);
    connect(&m_fileReceiver,&FileReceiver::receiveError,this,&RemoteServer::receiveError);
}

RemoteServer::~RemoteServer()
{
    emit stopCaptureRequested();

   
   if(m_captureThread){
        m_captureThread->quit();
        m_captureThread->wait();
    }

    MouseController::releaseAllButtons();
}

bool RemoteServer::listen(quint16 port)
{
    const bool ok = m_server.listen(QHostAddress::Any, port);
    if (ok) {
        qDebug() << "[server] listening on port" << port;
    } else {
        qWarning() << "[server] listen failed:" << m_server.errorString();
    }
    return ok;
}

void RemoteServer::onNewConnection()
{

    while (QTcpSocket *client = m_server.nextPendingConnection()) {

        //单连接限制
        if(m_currClientSocket && m_currClientSocket->state()==QAbstractSocket::SocketState::ConnectedState)
        {
            qWarning()<< "[server] reject client, already connected:"
                      << client->peerAddress().toString()
                      << client->peerPort();

                      client->disconnectFromHost();
                      client->deleteLater();
                      continue;
        }

        qDebug() << "[server] client connected:" << client->peerAddress().toString() << client->peerPort();

        m_currClientSocket=client;


        emit clientConnected(
            client->peerAddress().toString(),
            client->peerPort(),
            client->localAddress().toString(),
            client->localPort()
        );


        connect(client, &QTcpSocket::readyRead, this, &RemoteServer::onReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &RemoteServer::onClientDisconnected);
    }
}

void RemoteServer::onReadyRead()
{
    auto *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) {
        return;
    }

    m_codec.appendData(client->readAll());
    //解包
    while (auto packet = m_codec.takePacket()) {
        qDebug() << "[server] packet received, type ="
                 << static_cast<quint16>(packet->type)
                 << "sequence =" << packet->sequence
                 << "payload =" << packet->payload.size();

        //收到心跳包后启动让截图子线程启动
        if(packet->type==MessageType::Heartbeat){
            if(!m_captureStarted){
                emit startCaptureRequested(16);
                m_captureStarted=true;
            }
            
            //发送一个心跳包回应
            if(m_currClientSocket && m_currClientSocket->state()==QAbstractSocket::ConnectedState){
                sendPacket(MessageType::HeartbeatAck,QByteArray(),packet->sequence);
                qDebug() << "[server] heartbeat ack sent";
            }

        }

        
        //收到鼠标移动事件
        else if(packet->type==MessageType::MouseMove){
            MouseController::moveFromPayload(packet->payload);
        }

        //收到鼠标滚轮事件
        else if(packet->type==MessageType::MouseWheel){
            MouseController::wheelFromPayload(packet->payload);
        }

        else if(packet->type==MessageType::MousePress){
            MouseController::pressFromPayload(packet->payload);
        }

        else if(packet->type==MessageType::MouseRelease){
            MouseController::releaseFromPayload(packet->payload);
        }


        //键盘键入
        else if(packet->type==MessageType::KeyEvent){
            keyboardController::keyFromPayload(packet->payload);
        }

        //收到文件开始数据包
        else if(packet->type==MessageType::FileBegin){
            m_fileReceiver.handleFileBegin(packet->payload);
        }

        //收到文件数据包
        else if(packet->type==MessageType::FileData){
            m_fileReceiver.handleFileData(packet->payload);
        }
        
        else if(packet->type==MessageType::FileEnd){
            m_fileReceiver.handleFileEnd(packet->payload);
        }
    }
}

void RemoteServer::onClientDisconnected()
{
    auto *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) {
        return;
    }

    qDebug() << "[server] client disconnected";

    emit stopCaptureRequested();
    m_captureStarted = false;
    MouseController::releaseAllButtons();

    if (client == m_currClientSocket) {
        m_currClientSocket = nullptr;
        emit clientDisconnected();
    }

    client->deleteLater();
}

bool RemoteServer::sendPacket(RemoteLink::MessageType type, const QByteArray &payload, quint32 sequence)
{
    if (!m_currClientSocket ||
        m_currClientSocket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    Packet packet;
    packet.type=type;
    packet.payload=payload;
    packet.sequence=sequence;

    return m_currClientSocket->write(PacketCodec::encode(packet)) !=-1;
}

void RemoteServer::sendServerKick()
{
    if (sendPacket(MessageType::ServerKick, QByteArray(), m_serverKickSequence++)) {
        m_currClientSocket->flush();
    }
}

void RemoteServer::cleanupClient(bool notifyClient)
{
    emit stopCaptureRequested();
    m_captureStarted = false;
    MouseController::releaseAllButtons();

    if (!m_currClientSocket) {
        return;
    }

    if (notifyClient) {
        sendServerKick();
    }

    disconnect(m_currClientSocket, nullptr, this, nullptr);
    m_currClientSocket->disconnectFromHost();
    m_currClientSocket->deleteLater();
    m_currClientSocket = nullptr;

    emit clientDisconnected();
}

void RemoteServer::disconnectClient()
{
    cleanupClient(true);
}

void RemoteServer::stop()
{
    m_server.close();
    cleanupClient(true);
}
