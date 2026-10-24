#include "CaptureWorker.h"
#include "ScreenCapture.h"

CaptureWorker::CaptureWorker(QObject *partent)
    :QObject(partent)
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
            QImage frame = sc.captureFrame();
            if (frame.isNull()) {
                return;
            }

            const QByteArray frameData = ScreenCapture::encodeJpeg(frame, 40);
            if (!frameData.isEmpty()) {
                emit frameReady(frameData);
            }
        });
    }

    if (!m_timer->isActive()) {
        m_timer->start(intervalMS);
    }
}

void CaptureWorker::stopCaptureWork()
{
    if(m_timer){
        m_timer->stop();
    }
}
