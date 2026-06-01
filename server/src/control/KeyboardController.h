#pragma once

#include <QObject>


class keyboardController:public QObject{
    Q_OBJECT

public:
    //从网络数据包解析并执行键盘按键操作
    static void keyFromPayload(const QByteArray &payload);

private:
    //将Qt键码转换为Windows虚拟键码(VK)
    static int qtKeyToVK(int qtkey);

};