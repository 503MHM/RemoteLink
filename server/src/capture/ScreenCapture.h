#pragma once

#include <QByteArray>

class ScreenCapture
{

public:
    ScreenCapture();
    ~ScreenCapture();

    static QByteArray captureJpeg(int qulity=60);

};
