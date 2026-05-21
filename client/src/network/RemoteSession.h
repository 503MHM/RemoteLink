#pragma once

#include <QObject>
#include <QTcpSocket>

class RemoteSession : public QObject
{
    Q_OBJECT

public:
    explicit RemoteSession(QObject *parent = nullptr);

    void connectToServer(const QString &address, quint16 port);
    void sendHeartbeat();

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket m_socket;
    quint32 m_sequence = 1;
};
