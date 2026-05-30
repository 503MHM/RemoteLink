#pragma once

#include <QObject>
#include <QImage>

class FrameDecodeWorker : public QObject{
    Q_OBJECT

public:
    FrameDecodeWorker(QObject *partent=nullptr);

public slots:
    //解码帧数据
    void decodeFrame(const QByteArray &jpegData);

signals:
    void frameDecoded(const QImage &image);
    void decodeFailed();

};