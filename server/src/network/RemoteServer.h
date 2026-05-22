#pragma once

#include "protocol/PacketCodec.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class RemoteServer : public QObject
{
    Q_OBJECT

public:
    explicit RemoteServer(QObject *parent = nullptr);

    bool listen(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QTcpServer m_server;
    RemoteLink::PacketCodec m_codec;
    QTcpSocket *m_currClientSocket;
    QTimer m_frameTimer;
    quint32 m_frameSequence;
};
