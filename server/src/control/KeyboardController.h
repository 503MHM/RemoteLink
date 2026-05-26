#pragma once

#include <QObject>


class keyboardController:public QObject{
    Q_OBJECT

public:
    static void keyFromPayload(const QByteArray &payload);

private:
    static int qtKeyToVK(int qtkey);



};