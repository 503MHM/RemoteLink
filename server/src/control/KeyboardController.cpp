#include "KeyboardController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <windows.h>


void keyboardController::keyFromPayload(const QByteArray & payload)
{
    //1.解析json
    QJsonDocument doc=QJsonDocument::fromJson(payload);
    if(!doc.isObject()){
        return;
    }
    QJsonObject obj=doc.object();

    int key=obj.value("key").toInt();
    int modifiers=obj.value("modifiers").toInt();
    bool pressed=obj.value("pressed").toBool();
    QString text=obj.value("text").toString();

    //2.把 Qt 的 key 转成 Windows 的 virtual-key code
    int vk=qtKeyToVK(key);
    if(vk==0){
        qWarning() << "[KeyboardController] unsupported key:" << key;
        return;
    }


    //3.用 SendInput 发送 key down / key up。
#ifdef Q_OS_WIN
    INPUT input;
    ZeroMemory(&input,sizeof(INPUT));

    input.type=INPUT_KEYBOARD;
    input.ki.wVk=static_cast<WORD>(vk);

    if(!pressed){
        input.ki.dwFlags=KEYEVENTF_KEYUP;
    }

    UINT sent=SendInput(1,&input,sizeof(INPUT));
    if(sent!=1){
        qWarning() << "[KeyboardController] SendInput failed:" << GetLastError();
    }

#elif
    Q_UNUSED(vk)
    qWarning() << "[KeyboardController] keyboard input is only implemented on Windows";
#endif

}

int keyboardController::qtKeyToVK(int qtkey)
{

#ifdef Q_OS_WIN
    //A~Z
    if(qtkey>=Qt::Key_A && qtkey<=Qt::Key_Z){
        return 'A'+(qtkey-Qt::Key_A);
    }

    //0~9
    if(qtkey>=Qt::Key_0 && qtkey<=Qt::Key_9){
        return '0'+(qtkey-Qt::Key_0);
    }

    switch (qtkey) {
        case Qt::Key_Space:
            return VK_SPACE;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return VK_RETURN;
        case Qt::Key_Backspace:
            return VK_BACK;
        case Qt::Key_Tab:
            return VK_TAB;
        case Qt::Key_Escape:
            return VK_ESCAPE;
    
        case Qt::Key_Left:
            return VK_LEFT;
        case Qt::Key_Right:
            return VK_RIGHT;
        case Qt::Key_Up:
            return VK_UP;
        case Qt::Key_Down:
            return VK_DOWN;
    
        case Qt::Key_Delete:
            return VK_DELETE;
        case Qt::Key_Home:
            return VK_HOME;
        case Qt::Key_End:
            return VK_END;
        case Qt::Key_PageUp:
            return VK_PRIOR;
        case Qt::Key_PageDown:
            return VK_NEXT;
    
        case Qt::Key_Shift:
            return VK_SHIFT;
        case Qt::Key_Control:
            return VK_CONTROL;
        case Qt::Key_Alt:
            return VK_MENU;
    
        case Qt::Key_F1:
            return VK_F1;
        case Qt::Key_F2:
            return VK_F2;
        case Qt::Key_F3:
            return VK_F3;
        case Qt::Key_F4:
            return VK_F4;
        case Qt::Key_F5:
            return VK_F5;
        case Qt::Key_F6:
            return VK_F6;
        case Qt::Key_F7:
            return VK_F7;
        case Qt::Key_F8:
            return VK_F8;
        case Qt::Key_F9:
            return VK_F9;
        case Qt::Key_F10:
            return VK_F10;
        case Qt::Key_F11:
            return VK_F11;
        case Qt::Key_F12:
            return VK_F12;
    
        default:
            return 0;
        }


#elif
    Q_UNUSED(qtKey)
    return 0;
#endif
}