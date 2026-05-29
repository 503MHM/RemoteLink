#include "MouseController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QScreen>
#include <QtGlobal>

#include <windows.h>

// bool MouseController::clickFromPayload(const QByteArray & payload)
// {

//     int x = 0;
//     int y = 0;

//     if (!positionFromPayload(payload, x, y)) {
//         return false;
//     }

//     QJsonDocument doc = QJsonDocument::fromJson(payload);
//     if (!doc.isObject()) {
//         return false;
//     }
//     QJsonObject obj = doc.object();
//     QString btn = obj.value("button").toString();

//     SetCursorPos(x, y);

//     if (btn == "left") {
//         mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
//         mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
//     } else if (btn == "right") {
//         mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
//         mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
//     }

//     return true;
// }

bool MouseController::moveFromPayload(const QByteArray & payload)
{

    int x = 0;
    int y = 0;

    if (!positionFromPayload(payload, x, y)) {
        return false;
    }

    SetCursorPos(x, y);
    //qDebug()<<"[server] mouse move:"<<x<<y;

    return true;
}

bool MouseController::wheelFromPayload(const QByteArray &payload)
{

    int x=0;
    int y=0;

    if(!positionFromPayload(payload,x,y)){
        return false;
    }

    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()) return false;
    QJsonObject obj=doc.object();
    int delatY=obj.value("deltaY").toInt();

    SetCursorPos(x,y);
    mouse_event(MOUSEEVENTF_WHEEL,0,0,delatY,0);


    return true;
}

bool MouseController::pressFromPayload(const QByteArray &payload)
{

    //解析json
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        return false;
    }
    QJsonObject obj=doc.object();

    QString btn=obj.value("button").toString();
    int x=-1;
    int y=-1;

    if(!positionFromPayload(payload,x,y)){
        return false;
    }
    if(x==-1 || y==-1) return false;

    if(btn=="left"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    }
    else if(btn=="right"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);
    }
    else if(btn=="middle"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_MIDDLEDOWN,0,0,0,0);
    }

    return true;
}

bool MouseController::releaseFromPayload(const QByteArray &payload)
{

    //解析json
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        return false;
    }
    QJsonObject obj=doc.object();

    QString btn=obj.value("button").toString();
    int x=-1;
    int y=-1;

    if(!positionFromPayload(payload,x,y)){
        return false;
    }
    if(x==-1 || y==-1) return false;

    if(btn=="left"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    }
    else if(btn=="right"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);
    }
    else if(btn=="middle"){
        SetCursorPos(x,y);
        mouse_event(MOUSEEVENTF_MIDDLEUP,0,0,0,0);
    }

    return true;
}

void MouseController::releaseAllButtons()
{

    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);
    mouse_event(MOUSEEVENTF_MIDDLEUP,0,0,0,0);
}

bool MouseController::positionFromPayload(const QByteArray & payload, int & x, int & y)
{

    //解析json文本
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        return false;
    }
    QJsonObject obj=doc.object();

    double xRatio=obj.value("xRatio").toDouble();
    double yRatio=obj.value("yRatio").toDouble();

    if (xRatio < 0.0 || xRatio > 1.0 || yRatio < 0.0 || yRatio > 1.0) {
        return false;
    }

    //获取server屏幕尺寸
    QScreen *screen=QGuiApplication::primaryScreen();
    if(!screen) return false;

    //换算坐标
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    x = qBound(0, static_cast<int>(xRatio * screenW), screenW - 1);
    y = qBound(0, static_cast<int>(yRatio * screenH), screenH - 1);

    return true;
}
