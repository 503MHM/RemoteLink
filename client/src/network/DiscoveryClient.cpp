#include "DiscoveryClient.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QDebug>

DiscoveryClient::DiscoveryClient(QObject *parent):
    QObject(parent)
{
    m_socket=new QUdpSocket(this);
}

DiscoveryClient::~DiscoveryClient()
{
}

bool DiscoveryClient::start(quint16 discoveryPort)
{
    m_discoveryPort=discoveryPort;

    //绑定随即分配的端口
    bool bindok=m_socket->bind(QHostAddress::AnyIPv4,0);
    if(!bindok) {
        qDebug()<<"[DiscoverClient]UDP bind failed!"<<m_socket->errorString();
        return false;
    }

    connect(m_socket,&QUdpSocket::readyRead,this,&DiscoveryClient::onReadyread);

    return true;
}

void DiscoveryClient::search()
{
    //创建json对象
    QJsonObject obj;
    obj["type"]="DISCOVERY";
    obj["clientName"]="RemoteLink Client";

    //转为字节数组发送
    QByteArray sendData=QJsonDocument(obj).toJson(QJsonDocument::Compact);

    //发送
    m_socket->writeDatagram(sendData,QHostAddress::Broadcast,m_discoveryPort);
}


void DiscoveryClient::onReadyread(){
    while (m_socket->hasPendingDatagrams())
    {
        QNetworkDatagram recvData=m_socket->receiveDatagram();
        
        QByteArray data=recvData.data();                            //json内容
        QHostAddress senderAddress=recvData.senderAddress();        //发送者的IP
        
        QJsonDocument doc=QJsonDocument::fromJson(data);
        if(!doc.isObject()) continue;

        QJsonObject obj=doc.object();
        if(obj.value("type").toString()!="DISCOVERY_REPLY"){
            continue;
        }

        QString deviceName=obj.value("deviceName").toString();
        quint16 tcpPort=static_cast<quint16>(obj.value("tcpPort").toInt());
        QString version=obj.value("version").toString();
        
        qDebug()<<"[client] discovered:"
                <<deviceName
                <<tcpPort
                <<version
                <<senderAddress.toString();

        //发现设备添加到ui
        emit serverFound(senderAddress.toString(),tcpPort,deviceName);
    
    }
    
}