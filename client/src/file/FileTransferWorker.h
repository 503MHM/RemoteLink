#pragma once

#include <QObject>
#include <QFile>
#include <QFileInfo>

class FileTransferWorker : public QObject
{
    Q_OBJECT
private:
    QFile m_file;
    QFileInfo m_fileInfo;
    QString m_fileId;
    qint64 m_sentBytes=0;
    quint32 m_chunkIndex=0;
    
public:
    FileTransferWorker(QObject *partent=nullptr);
    ~FileTransferWorker();

public slots:
    void startSend(const QString filePath);
    void readNextChunk();

signals:
    void fileBeginReady(const QByteArray &beginPyload);
    void fileEndReady(const QByteArray &endPyload);
    void fileDataReady(const QByteArray &chunk);
    void progressChanged(quint64 sent,quint64 total);
    void finished();
    void errorOccurred(const QString &message);
};
