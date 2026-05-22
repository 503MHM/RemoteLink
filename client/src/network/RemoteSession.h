#pragma once

#include <QObject>
#include <QTcpSocket>

#include "protocol/PacketCodec.h"

class RemoteSession : public QObject
{
    Q_OBJECT

public:
    explicit RemoteSession(QObject *parent = nullptr);

    void connectToServer(const QString &address, quint16 port);
    void sendHeartbeat();


signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void frameReceived(const QImage &image);


private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReadyread();

private:
    QTcpSocket m_socket;
    quint32 m_sequence = 1;

    RemoteLink::PacketCodec m_codec;
};
