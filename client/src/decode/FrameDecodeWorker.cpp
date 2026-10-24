#include "FrameDecodeWorker.h"
#include <QImage>

FrameDecodeWorker::FrameDecodeWorker(QObject *partent)
    :QObject(partent)
{
}



void FrameDecodeWorker::decodeFrame(const QByteArray &jpegData){
    QImage image;
    if (!image.loadFromData(jpegData, "JPG")) {
        emit decodeFailed();
        return;
    }

    emit frameDecoded(image);
}
