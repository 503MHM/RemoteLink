#include "DiscoveryResponder.h"
#include "protocol/NetworkConstants.h"
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostInfo>

DiscoveryResponder::DiscoveryResponder(QObject *parent):
    QObject(parent)
{
    m_socket=new QUdpSocket(this);
    connect(m_socket,&QUdpSocket::readyRead,this,&DiscoveryResponder::onReadyRead);    

}

DiscoveryResponder::~DiscoveryResponder()
{
}


bool DiscoveryResponder::start(quint16 discoveryPort, quint16 tcpPort)
{
    //启动udp监听端口
    bool flag=m_socket->bind(QHostAddress::Any,discoveryPort,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if(!flag) return false;
    //监听udp数据
    m_tcpPort=tcpPort;
    qDebug() << "[server] discovery listening on port" << discoveryPort;

    return flag;
}

void DiscoveryResponder::stop()
{
    if(m_socket){
        m_socket->close();
    }
}

bool DiscoveryResponder::isRunning()
{
    return m_socket && m_socket->state()==QAbstractSocket::SocketState::BoundState;
}

void DiscoveryResponder::onReadyRead(){

    while(m_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram=m_socket->receiveDatagram();
        QByteArray recvData=datagram.data();
        QHostAddress senderAddress=datagram.senderAddress();
        quint16 senderPort=datagram.senderPort();
        qDebug() << "[server] discovery request from" << senderAddress.toString() << senderPort;


        //json文档
        QJsonDocument doc=QJsonDocument::fromJson(recvData);
        if(!doc.isObject()) continue;

        //json对象
        QJsonObject obj=doc.object();
        if (obj.value("type").toString() !=
            QString::fromLatin1(RemoteLink::DiscoveryType)) {
            continue;
        }

        
        //回复client
        //创建一个json对象
        QJsonObject reply;
        reply["type"] = QString::fromLatin1(RemoteLink::DiscoveryReplyType);
        reply["deviceName"]=QHostInfo::localHostName();
        reply["tcpPort"]=m_tcpPort;
        reply["version"]="1.0";

        //转换成字节数组
        QByteArray replyData=QJsonDocument(reply).toJson(QJsonDocument::Compact);

        //回复client端
        m_socket->writeDatagram(replyData,senderAddress,senderPort);

    }

}
