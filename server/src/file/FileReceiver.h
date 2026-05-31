#pragma once

#include <QObject>
#include <QFile>

class FileReceiver : public QObject{

    Q_OBJECT

public:
    FileReceiver(QObject *partent=nullptr);

    bool handleFileBegin(const QByteArray &payload);
    bool handleFileData(const QByteArray &payload);
    bool handleFileEnd(const QByteArray &payload);

signals:
    void receiveStarted(const QString &fileName,quint64 totalBytes);
    void receiveProgress(quint64 receivedBytes,quint64 totalBytes);
    void receiveFinished(bool ok,QString filePath);
    void receiveError(const QString &message);

private:
    QFile m_file;
    quint64 m_totalBytes=0;
    quint64 m_receivedBytes=0;
    QString m_fileId;

};