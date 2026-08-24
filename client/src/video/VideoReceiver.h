#pragma once

#include<QObject>
#include<QByteArray>


class VideoReceiver : public QObject
{
    Q_OBJECT

public:
    explicit VideoReceiver(QObject *parent=nullptr);

public slots:
    void onFrameDataReceived(const QByteArray &payload);

private:

signals:
    void jpegDataReceived(const QByteArray &jpegData);

};