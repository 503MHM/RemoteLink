#pragma once
#include <QObject>
#include <QUdpSocket>

class DiscoveryResponder:public QObject
{
    Q_OBJECT
public:
    DiscoveryResponder(QObject *parent=nullptr);
    ~DiscoveryResponder();
    bool start(quint16 discoveryPort,quint16 tcpPort);


private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket;
    quint16 m_tcpPort=45454;
};

