#include "FrameDecodeWorker.h"
#include <QDebug>
#include <QImage>

FrameDecodeWorker::FrameDecodeWorker(QObject *partent)
    :QObject(partent)
{
}

void FrameDecodeWorker::decodeFrame(const QByteArray &jpegData){
    if (!m_statsTimer.isValid()) {
        m_statsTimer.start();
    }

    QElapsedTimer decodeTimer;
    decodeTimer.start();

    QImage image;
    if (!image.loadFromData(jpegData, "JPG")) {
        ++m_failedFrames;
        logDecodeStatsIfDue();
        emit decodeFailed();
        return;
    }

    const qint64 decodeUs = decodeTimer.nsecsElapsed() / 1000;
    ++m_decodedFrames;
    m_decodedBytes += static_cast<quint64>(jpegData.size());
    m_decodeTimeUs += decodeUs;
    m_decodeMaxUs = qMax(m_decodeMaxUs, decodeUs);
    logDecodeStatsIfDue();
    emit frameDecoded(image);
}

void FrameDecodeWorker::logDecodeStatsIfDue()
{
    const qint64 intervalMs = m_statsTimer.elapsed();
    if (intervalMs < 1000) {
        return;
    }

    const double seconds = intervalMs / 1000.0;
    const double averageMs = m_decodedFrames
        ? static_cast<double>(m_decodeTimeUs) / m_decodedFrames / 1000.0 : 0.0;
    const double averageKiB = m_decodedFrames
        ? static_cast<double>(m_decodedBytes) / m_decodedFrames / 1024.0 : 0.0;

    qInfo() << "[decode] fps=" << m_decodedFrames / seconds
            << "failed=" << m_failedFrames
            << "avg_ms=" << averageMs
            << "max_ms=" << m_decodeMaxUs / 1000.0
            << "avg_jpeg_kib=" << averageKiB;

    m_statsTimer.restart();
    m_decodedFrames = 0;
    m_failedFrames = 0;
    m_decodedBytes = 0;
    m_decodeTimeUs = 0;
    m_decodeMaxUs = 0;
}
