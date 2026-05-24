#pragma once

#include <QObject>


class FileTransferWorker : public QObject
{
    Q_OBJECT
private:
    
public:
    FileTransferWorker(QObject *partent=nullptr);
    ~FileTransferWorker();

    void startSend(const QString filePath);

signals:
    void fileBeginReady(const QByteArray &beginPyload);
    void fileEndReady(const QByteArray &endPyload);
    void fileDataReady(const QByteArray &chunk);
    void progressChanged(quint64 sent,quint64 total);
    void finished();
    void errorOccurred(const QString &message);
};
