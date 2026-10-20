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

    connect(&m_socket,&QTcpSocket::bytesWritten,this,[this](quint64 bytes){
        constexpr qint64 LowWatrmark=512*1024;      //低水位512KB
    
        if(m_socket.bytesToWrite()<=LowWatrmark){
            //低于低水位就让子线程继续读取下一个数据块
            emit writeBufferBeLowWatermark();
        }
    });


    //定时发送心跳包
    connect(&m_heartbeatTimer,&QTimer::timeout,this,[this](){
        if(m_socket.state()!= QAbstractSocket::ConnectedState){
            m_heartbeatTimer.stop();
            return;
        }

        if(m_lastPongTime.isValid() && m_lastPongTime.elapsed()>15000){
            qWarning()<<"[client] heartbeat timeout";
            
            m_heartbeatTimer.stop();
            m_socket.abort();
            startReconnect();
            return;
        }

        sendHeartbeat();
    });

    //定时重连
    connect(&m_reconnectTimer,&QTimer::timeout,this,[this](){
        if(m_socket.state()==QAbstractSocket::ConnectedState){
            m_reconnectTimer.stop();
            return;
        }

        if(m_reconnectCount>=MaxReconnectCount){
            m_reconnectTimer.stop();
            emit errorOccurred("[client] reconnect failed");
            return;
        }

        m_reconnectCount++;

        qDebug()<<"[client] reconnecting..."<< m_reconnectCount << "/" << MaxReconnectCount;
    
        m_socket.abort();
        m_socket.connectToHost(m_lastAddress,m_lastPort);
    });
}

void RemoteSession::connectToServer(const QString &address, quint16 port)
{
    qDebug() << "[client] connecting to" << address << port;
    m_lastAddress=address;
    m_lastPort=port;
    m_reconnectCount=0;
    m_manualDisconnect=false;
    m_socket.connectToHost(address, port);
}

void RemoteSession::sendHeartbeat()
{
    QByteArray payload("Heartbeat packet");
    sendPacket(RemoteLink::MessageType::Heartbeat,payload);
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

void RemoteSession::sendMousePress(double xRatio, double yRatio, const QString &btn)
{
    //构建json
    QJsonObject obj;
    obj["xRatio"]=xRatio;
    obj["yRatio"]=yRatio;
    obj["button"]=btn;

    QByteArray payload=QJsonDocument(obj).toJson();

    sendPacket(MessageType::MousePress,payload);
}

void RemoteSession::sendMouseRelease(double xRatio, double yRatio, const QString &btn)
{
    //构建json
    QJsonObject obj;
    obj["xRatio"]=xRatio;
    obj["yRatio"]=yRatio;
    obj["button"]=btn;

    QByteArray payload=QJsonDocument(obj).toJson();

    sendPacket(MessageType::MouseRelease,payload);
}

bool RemoteSession::sendPacket(RemoteLink::MessageType type, const QByteArray &payload)
{
    if(m_socket.state()!=QAbstractSocket::ConnectedState)
    {
        return false;
    }

    //构建packet
    Packet packet;
    packet.type=type;
    packet.sessionId=m_sessionId;
    packet.sequence=m_sequence;
    packet.payload=payload;

    //打包包头和载荷
    QByteArray sendData=PacketCodec::encode(packet);

    if(sendData.isEmpty())
    {
        return false;
    }

    //发送
    if(m_socket.write(sendData)==-1)
    {
        return false;
    }

    //发送成功递增序列号
    ++m_sequence;
    return true;
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

void RemoteSession::sendKeyEvent(int key, int modifiers, bool pressed, QString text)
{
    QJsonObject obj;
    obj["key"]=key;
    obj["modifiers"]=modifiers;
    obj["pressed"]=pressed;
    obj["text"]=text;

    QByteArray payload=QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Compact);

    //
    sendPacket(MessageType::KeyEvent,payload);
}

qint64 RemoteSession::bytesToWrite() const
{
    return m_socket.bytesToWrite();
}

bool RemoteSession::isConnected()
{
    if(m_socket.state()==QAbstractSocket::SocketState::ConnectedState){
        return true;
    }

    return false;
}

void RemoteSession::disConnectFromHost()
{
    m_manualDisconnect=true;
    m_reconnectTimer.stop();
    m_heartbeatTimer.stop();

    m_socket.disconnectFromHost();
}

void RemoteSession::onConnected()
{
    qDebug() << "[client] connected";

    m_sessionId = 0;
    m_sequence = 1;

    m_reconnectCount=0;
    m_reconnectTimer.stop();
    m_manualDisconnect=false;

    emit this->connected();
    sendHeartbeat();

    m_lastPongTime.restart();
    m_heartbeatTimer.start(5000);
}

void RemoteSession::onDisconnected()
{
    m_heartbeatTimer.stop();
    m_codec.clear();
    m_sessionId=0;

    qDebug() << "[client] disconnected";
    emit this->disconnected();

    if(!m_manualDisconnect){
        startReconnect();
    }
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
        if(packet->type==MessageType::HeartbeatAck)
        {
            if(packet->sessionId==0)
            {
                qWarning()<<"[client] heartbeat ack has invalid sessionId";
                continue;
            }

            if(m_sessionId==0)
            {
                m_sessionId=packet->sessionId;
                emit sessionEstablished(m_sessionId);
            }
            else if(packet->sessionId!=m_sessionId)
            {
                qWarning()<<"[client] heartbeat ack received";
                m_lastPongTime.restart();
                continue;
            }
        }


        //校验sessionId
        if(m_sessionId!=0 && packet->sessionId!=m_sessionId){
            continue;
        }
        
        if(packet->type==MessageType::FrameData){

            emit frameDataReceived(packet->payload);

        }

        else if(packet->type==MessageType::ServerKick){
            m_manualDisconnect=true;
            m_reconnectTimer.stop();
            m_heartbeatTimer.stop();
            m_socket.disconnectFromHost();
        }
    }
    
}

void RemoteSession::startReconnect()
{
    if(m_manualDisconnect){
        return;
    }

    if(m_lastAddress.isEmpty() || m_lastPort==0){
        return;
    }

    if(m_reconnectCount>=MaxReconnectCount){
        return;
    }

    if(!m_reconnectTimer.isActive()){
        m_reconnectTimer.start(3000);
    }

}
