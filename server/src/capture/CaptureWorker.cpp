#include "CaptureWorker.h"
#include "ScreenCapture.h"
#include <QElapsedTimer>
#include <QDebug>

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

            QByteArray frameData = sc.captureJpeg(100);

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
