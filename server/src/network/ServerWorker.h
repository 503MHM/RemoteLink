#pragma once

#include <QObject>


class RemoteServer;


class ServerWorker:public QObject{
    Q_OBJECT

public:
    ServerWorker(QObject *partent=nullptr);
    ~ServerWorker();


public slots:
    void start(quint16 port);
    void stop();


signals:
    void started(bool ok);
    void stoped();
    void errorOccurred(const QString &message);

private:

    RemoteServer *m_server=nullptr;

};