#include "RemoteServer.h"
#include "../capture/ScreenCapture.h"
#include "protocol/PacketCodec.h"
#include "./control/MouseController.h"

#include <QDebug>

using namespace RemoteLink;

RemoteServer::RemoteServer(QObject *parent)
    : QObject(parent)
    ,m_frameSequence(0)
    ,m_currClientSocket(nullptr)
    {
    connect(&m_server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection);
    
    //定时发送截图
    connect(&m_frameTimer,&QTimer::timeout,this,[=](){
        if(!m_currClientSocket || (m_currClientSocket->state()!=QAbstractSocket::SocketState::ConnectedState)) return;

        QByteArray jpegData=ScreenCapture::captureJpeg(50);
        if(jpegData.isEmpty()) return;

        //打包
        Packet packet;
        packet.type=MessageType::FrameData;
        packet.sequence=m_frameSequence++;
        packet.payload=jpegData;
        QByteArray sendData=PacketCodec::encode(packet);

        m_currClientSocket->write(sendData);
    });
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
        qDebug() << "[server] client connected:" << client->peerAddress().toString() << client->peerPort();

        m_currClientSocket=client;

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

        //收到心跳包后启动定时器
        if(packet->type==MessageType::Heartbeat){
            m_frameTimer.start(200);
        }

        //收到鼠标点击事件
        else if(packet->type==MessageType::MouseClick){
            //qDebug()<<packet->payload;
            MouseController::clickFromPayload(packet->payload);
        }
        
        //收到鼠标移动事件
        else if(packet->type==MessageType::MouseMove){
            MouseController::moveFromPayload(packet->payload);
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

    if (client == m_currClientSocket) {
        m_frameTimer.stop();
        m_currClientSocket = nullptr;
    }

    client->deleteLater();
}
