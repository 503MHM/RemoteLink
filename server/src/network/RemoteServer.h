#pragma once

#include "protocol/PacketCodec.h"
#include "file/FileReceiver.h"
#include "capture/CaptureWorker.h"
#include "video/VideoSender.h"
#include "video/UdpVideoSender.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QThread>
#include <QString>

class RemoteServer : public QObject
{
    Q_OBJECT

public:
    explicit RemoteServer(QObject *parent = nullptr);
    ~RemoteServer();

    bool listen(quint16 port);
    void disconnectClient();

    void stop();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    bool sendPacket(RemoteLink::MessageType type,const QByteArray &payload,quint32 sequence);
    void sendServerKick();
    void cleanupClient(bool notifyClient);
    bool sendVideoFrame(const QByteArray &frameData);

signals:
    void startCaptureRequested(quint16 intervalMS);
    void stopCaptureRequested();

    void clientConnected(const QString clientIp, quint16 clientPort,const QString localIp, quint16 localPort);
    void clientDisconnected();

    //文件相关信号
    void receiveStarted(const QString &fileName,quint64 totalBytes);
    void receiveProgress(quint64 receivedBytes,quint64 totalBytes);
    void receiveFinished(bool ok,QString filePath);
    void receiveError(const QString &message);


private:
    QTcpServer m_server;
    QTcpSocket *m_currClientSocket;
    RemoteLink::PacketCodec m_codec;
    //quint32 m_frameSequence;

    quint32 m_sessionId=0;
    quint32 m_nextServerSequence=1;

    //quint32 m_nextVideoFrameId=1;

    FileReceiver m_fileReceiver;

    //截图子线程
    QThread *m_captureThread;
    CaptureWorker *m_captureWorker;

    //用于提示当前是否已启动截图
    bool m_captureStarted=false;

    VideoSender m_videoSender;

    UdpVideoSender m_udpVideoSender;

};
