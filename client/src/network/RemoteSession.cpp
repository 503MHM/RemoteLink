#include "RemoteSession.h"
#include "protocol/PacketCodec.h"
#include "protocol/MessageType.h"

#include <QNetworkProxy>
#include <QImage>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

using namespace RemoteLink;

RemoteSession::RemoteSession(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected, this, &RemoteSession::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &RemoteSession::onDisconnected);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &RemoteSession::onErrorOccurred);
    connect(&m_socket,&QTcpSocket::readyRead,this,&RemoteSession::onReadyread);

    m_socket.setProxy(QNetworkProxy::NoProxy);

    connect(&m_socket,&QTcpSocket::bytesWritten,this,&RemoteSession::bytesWritten);
}

void RemoteSession::connectToServer(const QString &address, quint16 port)
{
    qDebug() << "[client] connecting to" << address << port;
    m_socket.connectToHost(address, port);
}

void RemoteSession::sendHeartbeat()
{
    Packet packet;
    packet.type = MessageType::Heartbeat;
    packet.sequence = m_sequence++;
    packet.payload = QByteArrayLiteral("hello from client");

    const QByteArray data = PacketCodec::encode(packet);
    m_socket.write(data);
    qDebug() << "[client] heartbeat sent, bytes =" << data.size();
}

void RemoteSession::sendMouseClick(double xRatio, double yRatio,const QString &btn)
{
    //构建json文本
    QJsonObject obj;
    obj["button"]=btn;
    obj["xRatio"]=xRatio;
    obj["yRatio"]=yRatio;

    //将json文本转换为字节数组
    QByteArray payload=QJsonDocument(obj).toJson(QJsonDocument::Compact);

    //发送
    sendPacket(MessageType::MouseClick,payload);
}

void RemoteSession::sendMouseMove(double xRatio, double yRatio)
{
    QJsonObject obj;
    obj["xRatio"]=xRatio;
    obj["yRatio"]=yRatio;

    //将json文本转换为字节数组
    QByteArray payload=QJsonDocument(obj).toJson(QJsonDocument::Compact);

    //发送
    sendPacket(MessageType::MouseMove,payload);
}

bool RemoteSession::sendPacket(RemoteLink::MessageType type, const QByteArray &payload)
{
    //构建packet
    Packet packet;
    packet.type=type;
    packet.sequence=m_sequence++;
    packet.payload=payload;

    //打包包头和载荷
    QByteArray sendData=PacketCodec::encode(packet);

    if(m_socket.state()!=QAbstractSocket::ConnectedState){
        return false;
    }

    //发送
    ;

    return m_socket.write(sendData)!=-1;
}

void RemoteSession::sendMouseWheel(double xRatio, double yRatio, const int deltaY)
{
    QJsonObject obj;
    obj["xRatio"]=xRatio;
    obj["yRatio"]=yRatio;
    obj["deltaY"]=deltaY;

    //将json文本转换为字节数组
    QByteArray payload=QJsonDocument(obj).toJson(QJsonDocument::Compact);

    //发送
    sendPacket(MessageType::MouseWheel,payload);
}

qint64 RemoteSession::bytesToWrite() const
{
    return m_socket.bytesToWrite();
}

void RemoteSession::onConnected()
{
    qDebug() << "[client] connected";
    emit connected();
    sendHeartbeat();
}

void RemoteSession::onDisconnected()
{
    qDebug() << "[client] disconnected";
    emit disconnected();
}

void RemoteSession::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    qWarning() << "[client] socket error:" << m_socket.errorString();
    emit errorOccurred(m_socket.errorString());
}

void RemoteSession::onReadyread()
{
    QByteArray Recedata=m_socket.readAll();

    m_codec.appendData(Recedata);
    while (auto packet=m_codec.takePacket())
    {
        if(packet->type==MessageType::FrameData){

            emit frameDataReceived(packet->payload);

        }
    }
    
}
