#pragma once

#include <QObject>
#include <QFile>
#include <QElapsedTimer>

class FileReceiver : public QObject{

    Q_OBJECT

public:
    FileReceiver(QObject *partent=nullptr);

    /**
     * @brief 创建并打开文件，存储文件的相关信息
     * @param payload 文件传输初始数据,包含文件名、文件ID、总字节数等信息
     * @return bool 解析成功返回true,失败返回false
     */
    bool handleFileBegin(const QByteArray &payload);

    /**
     * @brief 写入文件
     * @param payload 文件数据块,包含文件分片数据
     * @return bool 写入成功返回true,失败返回false
     */
    bool handleFileData(const QByteArray &payload);

    /**
     * @brief 处理文件结束传输的信号
     * @param payload 文件结束数据,可能包含校验信息
     * @return bool 处理成功返回true,失败返回false
     */
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
    quint64 m_expectedChunkIndex=0;
    QElapsedTimer m_transferTimer;
    QElapsedTimer m_progressTimer;
    quint64 m_lastProgressBytes = 0;
};
