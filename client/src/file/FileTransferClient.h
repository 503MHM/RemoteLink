#pragma once 

#include <QObject>
#include "network/RemoteSession.h"
#include "file/FileTransferWorker.h"
#include <QThread>

class FileTransferClient:public QObject
{
    Q_OBJECT

private:
    FileTransferWorker *m_worker=nullptr;
    QThread *m_workerThread=nullptr;
    //bool m_running=false;

public:
    FileTransferClient(QObject *partent=nullptr);
    ~FileTransferClient();

    void sendFile(const QString &filePath,RemoteSession *session);

signals:
    void progressChanged(qint64 sentBytes,qint64 totalBytes);
    void finished();
    void errorOccurred(const QString &message);

};
