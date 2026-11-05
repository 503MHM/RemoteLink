#include "CaptureWorker.h"
#include "ScreenCapture.h"
#include <QDebug>
#include <QSize>
#include <QElapsedTimer>

namespace {
int configuredJpegQuality()
{
    bool ok = false;
    const int requested = qEnvironmentVariableIntValue("REMOTELINK_JPEG_QUALITY", &ok);
    return ok ? qBound(0, requested, 100) : 100;
}
}

CaptureWorker::CaptureWorker(QObject *partent)
    :QObject(partent)
    ,m_jpegQuality(configuredJpegQuality())
{
}

CaptureWorker::~CaptureWorker()
{
}


void CaptureWorker::startCaptureWork(quint16 intervalMS)
{
    if (!m_timer) {
        m_timer = new QTimer(this);

        connect(m_timer, &QTimer::timeout, this, [this]() {
            QElapsedTimer stageTimer;
            stageTimer.start();
            QImage frame = sc.captureFrame();
            const qint64 captureUs = stageTimer.nsecsElapsed() / 1000;
            ++m_captureAttempts;
            m_captureTimeUs += captureUs;
            m_captureMaxUs = qMax(m_captureMaxUs, captureUs);

            if (frame.isNull()) {
                // No desktop update was available during this capture tick.
            } else {
                ++m_captureFrames;
                m_lastFrameSize = frame.size();

                stageTimer.restart();
                const QByteArray frameData =
                    ScreenCapture::encodeJpeg(frame, m_jpegQuality);
                const qint64 encodeUs = stageTimer.nsecsElapsed() / 1000;
                m_encodeTimeUs += encodeUs;
                m_encodeMaxUs = qMax(m_encodeMaxUs, encodeUs);

                if (!frameData.isEmpty()) {
                    ++m_encodedFrames;
                    m_encodedBytes += static_cast<quint64>(frameData.size());
                    emit frameReady(frameData);
                }
            }

            if (m_metricsTimer.elapsed() >= 1000) {
                const qint64 intervalMs = m_metricsTimer.restart();
                const double seconds = intervalMs / 1000.0;
                const double captureFps = m_captureFrames / seconds;
                const double encodedFps = m_encodedFrames / seconds;
                const double captureAvgMs = m_captureAttempts
                    ? static_cast<double>(m_captureTimeUs) / m_captureAttempts / 1000.0 : 0.0;
                const double encodeAvgMs = m_captureFrames
                    ? static_cast<double>(m_encodeTimeUs) / m_captureFrames / 1000.0 : 0.0;
                const double jpegAvgKiB = m_encodedFrames
                    ? static_cast<double>(m_encodedBytes) / m_encodedFrames / 1024.0 : 0.0;

                qInfo() << "[capture] resolution=" << m_lastFrameSize
                        << "quality=" << m_jpegQuality
                        << "capture_fps=" << captureFps
                        << "jpeg_fps=" << encodedFps
                        << "capture_avg_ms=" << captureAvgMs
                        << "capture_max_ms=" << m_captureMaxUs / 1000.0
                        << "jpeg_avg_ms=" << encodeAvgMs
                        << "jpeg_max_ms=" << m_encodeMaxUs / 1000.0
                        << "jpeg_avg_kib=" << jpegAvgKiB;

                m_captureAttempts = 0;
                m_captureFrames = 0;
                m_encodedFrames = 0;
                m_encodedBytes = 0;
                m_captureTimeUs = 0;
                m_captureMaxUs = 0;
                m_encodeTimeUs = 0;
                m_encodeMaxUs = 0;
            }
        });
    }

    if (!m_timer->isActive()) {
        m_captureAttempts = 0;
        m_captureFrames = 0;
        m_encodedFrames = 0;
        m_encodedBytes = 0;
        m_captureTimeUs = 0;
        m_captureMaxUs = 0;
        m_encodeTimeUs = 0;
        m_encodeMaxUs = 0;
        m_lastFrameSize = {};
        m_metricsTimer.start();
        m_timer->start(intervalMS);
    }
}

void CaptureWorker::stopCaptureWork()
{
    if(m_timer){
        m_timer->stop();
    }
}
