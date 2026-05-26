#pragma once 

#include <QByteArray>

class MouseController{

public:
    static bool clickFromPayload(const QByteArray &payload);
    static bool moveFromPayload(const QByteArray &payload);
    static bool wheelFromPayload(const QByteArray &payload);

private:
    static bool positionFromPayload(const QByteArray &payload, int &x, int &y);
};