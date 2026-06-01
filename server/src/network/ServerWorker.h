#pragma once

#include <QObject>


class RemoteServer;


class ServerWorker:public QObject{
    Q_OBJECT

public:
    ServerWorker(QObject *partent=nullptr);
    ~ServerWorker();


public slots:
    //开启服务
    void start(quint16 port);
    
    //停止服务
    void stop();

    //断开和client连接
    void disconnectClient();

signals:
    void started(bool ok);
    void stoped();
    void errorOccurred(const QString &message);

    void clientConnected(const QString clientIp, quint16 clientPort,const QString localIp, quint16 localPort);
    void clientDisconnected();

    void receiveStarted(const QString &fileName,quint64 totalBytes);
    void receiveProgress(quint64 receivedBytes,quint64 totalBytes);
    void receiveFinished(bool ok,QString filePath);
    void receiveError(const QString &message);

private:

    RemoteServer *m_server=nullptr;

};