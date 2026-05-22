#include "ScreenCapture.h"
#include <QGuiApplication>
#include <QPixmap>
#include <QByteArray>
#include <QBuffer>
#include <QScreen>

ScreenCapture::ScreenCapture()
{
}

ScreenCapture::~ScreenCapture()
{
}


/// @brief 返回一张屏幕截图的字节数组
/// @param quality 图像质量
/// @return 
QByteArray ScreenCapture::captureJpeg(int quality)
{
    QScreen *screen=QGuiApplication::primaryScreen();
    if(!screen) return QByteArray();

    QPixmap pixmap=screen->grabWindow(0);

    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);

    pixmap.save(&buf,"JPG",quality);

    return data;
}
