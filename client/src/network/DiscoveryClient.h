#pragma once

#include<QObject>
#include<QUdpSocket>

class DiscoveryClient:public QObject
{
    Q_OBJECT
private:
    QUdpSocket *m_socket;
    quint16 m_discoveryPort=45455;

public:
    DiscoveryClient(QObject *parent=nullptr);
    ~DiscoveryClient();

    bool start(quint16 discoveryPort);
    void search();

signals:
    void serverFound(const QString &ip,quint16 tcpPort,const QString &deviceName);

private slots:
    void onReadyread();
};
