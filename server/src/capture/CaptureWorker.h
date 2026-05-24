#pragma once

#include <QObject>
#include <QTimer>

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

signals:
    void frameReady(const QByteArray &frameData);

};
