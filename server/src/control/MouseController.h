#pragma once 

#include <QByteArray>

class MouseController{

public:

    //从payload解析鼠标移动事件并执行
    static bool moveFromPayload(const QByteArray &payload);
    
    //滚轮事件
    static bool wheelFromPayload(const QByteArray &payload);
    
    //鼠标按下
    static bool pressFromPayload(const QByteArray &payload);
    
    //鼠标松开
    static bool releaseFromPayload(const QByteArray &payload);

    //释放所有鼠标按键
    static void releaseAllButtons();

private:

    //把坐标比例转换成真实坐标
    static bool positionFromPayload(const QByteArray &payload, int &x, int &y);
};