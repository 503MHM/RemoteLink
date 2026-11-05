#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QSize>

#include "ScreenCapture.h"

class CaptureWorker : public QObject{
    Q_OBJECT

public:
    CaptureWorker(QObject *partent=nullptr);
    ~CaptureWorker();

public slots:
    void startCaptureWork(quint16 intervalMS);
    void stopCaptureWork();
private:
    QTimer *m_timer=nullptr;
    ScreenCapture sc;
    QElapsedTimer m_metricsTimer;
    quint64 m_captureAttempts = 0;
    quint64 m_captureFrames = 0;
    quint64 m_encodedFrames = 0;
    quint64 m_encodedBytes = 0;
    qint64 m_captureTimeUs = 0;
    qint64 m_captureMaxUs = 0;
    qint64 m_encodeTimeUs = 0;
    qint64 m_encodeMaxUs = 0;
    QSize m_lastFrameSize;
    int m_jpegQuality = 100;

signals:
    void frameReady(const QByteArray &frameData);

};
