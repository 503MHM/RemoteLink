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
    bool m_cancelRequested=false;

public:
    FileTransferClient(QObject *partent=nullptr);
    ~FileTransferClient();

    //
    void sendFile(const QString &filePath,RemoteSession *session);
    //client 断开时停止文件读取，取消继续读取数据块
    void cancel();

signals:
    void progressChanged(qint64 sentBytes,qint64 totalBytes);
    void finished();
    void errorOccurred(const QString &message);

};
