#pragma once

#include "protocol/PacketCodec.h"
#include "file/FileReceiver.h"
#include "capture/CaptureWorker.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QThread>

class RemoteServer : public QObject
{
    Q_OBJECT

public:
    explicit RemoteServer(QObject *parent = nullptr);
    ~RemoteServer();

    bool listen(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QTcpServer m_server;
    QTcpSocket *m_currClientSocket;
    RemoteLink::PacketCodec m_codec;
    quint32 m_frameSequence;

    FileReceiver m_fileReceiver;

    QThread *m_captureThread;
    CaptureWorker *m_captureWorker;


signals:
    void startCaptureRequested(quint16 intervalMS);
    void stopCaptureRequested();

};
