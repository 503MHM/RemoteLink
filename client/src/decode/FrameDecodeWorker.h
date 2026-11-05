#pragma once

#include <QObject>
#include <QImage>
#include <QElapsedTimer>

class FrameDecodeWorker : public QObject{
    Q_OBJECT

public:
    FrameDecodeWorker(QObject *partent=nullptr);

public slots:
    //解码帧数据
    void decodeFrame(const QByteArray &jpegData);

private:
    void logDecodeStatsIfDue();

    QElapsedTimer m_statsTimer;
    quint64 m_decodedFrames = 0;
    quint64 m_failedFrames = 0;
    quint64 m_decodedBytes = 0;
    qint64 m_decodeTimeUs = 0;
    qint64 m_decodeMaxUs = 0;

signals:
    void frameDecoded(const QImage &image);
    void decodeFailed();

};
