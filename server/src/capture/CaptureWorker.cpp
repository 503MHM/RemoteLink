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
            // QElapsedTimer timer;
            // timer.start();

            QByteArray frameData = sc.captureJpeg(60);

            //qDebug()<< "[server] captureJpeg ms =" << timer.elapsed()
            //<< "size =" << frameData.size();

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
    m_timer->stop();
}
