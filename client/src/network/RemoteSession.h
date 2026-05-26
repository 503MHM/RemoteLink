#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QImage>

#include "protocol/PacketCodec.h"

class RemoteSession : public QObject
{
    Q_OBJECT

public:
    explicit RemoteSession(QObject *parent = nullptr);

    void connectToServer(const QString &address, quint16 port);
    void sendHeartbeat();
    void sendMouseClick(double xRatio,double yRatio,const QString &btn);
    void sendMouseMove(double xRatio,double yRatio);
    bool sendPacket(RemoteLink::MessageType type,const QByteArray &payload);

    void sendMouseWheel(double xRatio,double yRatio,const int deltaY);

    void sendKeyEvent(int key, int modifiers, bool pressed, QString text);

    qint64 bytesToWrite() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);

    void frameDataReceived(const QByteArray &frameData);

    void bytesWritten(qint64 bytes);


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
