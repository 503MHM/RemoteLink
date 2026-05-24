#pragma once 

#include <QObject>
#include "network/RemoteSession.h"


class FileTransferClient:public QObject
{
    Q_OBJECT

private:
    


public:
    FileTransferClient(QObject *partent=nullptr);
    ~FileTransferClient();

    void sendFile(const QString &filePath,RemoteSession *session);

signals:
    void progressChanged(qint64 sentBytes,qint64 totalBytes);
    void finished();
    void errorOccurred(const QString &message);

};
