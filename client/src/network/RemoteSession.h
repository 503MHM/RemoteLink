#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>

#include "protocol/PacketCodec.h"

class RemoteSession : public QObject
{
    Q_OBJECT

public:
    explicit RemoteSession(QObject *parent = nullptr);

    //连接服务端
    void connectToServer(const QString &address, quint16 port);
    
    //发送心跳包
    void sendHeartbeat();

    //发送鼠标相关操作
    void sendMouseMove(double xRatio,double yRatio);
    void sendMousePress(double xRatio, double yRatio, const QString &btn);
    void sendMouseRelease(double xRatio, double yRatio, const QString &btn);
    void sendMouseWheel(double xRatio,double yRatio,const int deltaY);
    
    //发送包
    bool sendPacket(RemoteLink::MessageType type,const QByteArray &payload);

    //发送键盘相关操作
    void sendKeyEvent(int key, int modifiers, bool pressed, QString text);

    //获取缓冲区的大小
    qint64 bytesToWrite() const;

    //获取连接状态
    bool isConnected();

    //断开连接
    void disConnectFromHost();
signals:

    void connected();
    void disconnected();
    void errorOccurred(const QString &message);

    //已解包的原始数据
    void frameDataReceived(const QByteArray &frameData);

    //socket 写缓冲低于低水位
    void writeBufferBeLowWatermark();

    void sessionEstablished(quint32 sessionId);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReadyread();

private:
    void startReconnect();


private:
    QTcpSocket m_socket;
    quint32 m_sequence = 1;
    quint32 m_sessionId = 0;

    RemoteLink::PacketCodec m_codec;

    QTimer m_heartbeatTimer;
    QElapsedTimer m_lastPongTime;

    QTimer m_reconnectTimer;
    QString m_lastAddress;
    quint16 m_lastPort=0;
    int m_reconnectCount=0;
    static constexpr int MaxReconnectCount=5;
    bool m_manualDisconnect=false;
};
