#include "CaptureWorker.h"
#include "ScreenCapture.h"

CaptureWorker::CaptureWorker(QObject *partent)
    :QObject(partent)
{   

    // m_timer=new QTimer(this);

    // connect(m_timer,&QTimer::timeout,this,[=](){
    //     QByteArray frameData=ScreenCapture::captureJpeg(60);
    //     emit frameReady(frameData);
    // });

}

CaptureWorker::~CaptureWorker()
{
}


void CaptureWorker::startCaptureWork(quint16 intervalMS)
{
    if (!m_timer) {
        m_timer = new QTimer(this);

        connect(m_timer, &QTimer::timeout, this, [this]() {
            QByteArray frameData = ScreenCapture::captureJpeg(60);
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
