#include "RemoteServer.h"

#include <QDebug>

using namespace RemoteLink;

RemoteServer::RemoteServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection);
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
    while (auto packet = m_codec.takePacket()) {
        qDebug() << "[server] packet received, type ="
                 << static_cast<quint16>(packet->type)
                 << "sequence =" << packet->sequence
                 << "payload =" << packet->payload;
    }
}

void RemoteServer::onClientDisconnected()
{
    auto *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) {
        return;
    }

    qDebug() << "[server] client disconnected";
    client->deleteLater();
}
