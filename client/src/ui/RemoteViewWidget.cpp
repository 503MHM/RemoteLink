#include "RemoteViewWidget.h"
#include <QMouseEvent>

RemoteViewWidget::RemoteViewWidget(QWidget *parent)
    :QLabel(parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    //打开鼠标跟踪
    setMouseTracking(true);

    //label获得键盘焦点
    setFocusPolicy(Qt::StrongFocus);

    m_moveTimer.start();
}

void RemoteViewWidget::mousePressEvent(QMouseEvent *ev)
{
    QPointF pos=ev->position();
    
    double xRatio=-1;
    double yRatio=-1;
    if(!positionToRatio(pos,xRatio,yRatio)) return;

    if(ev->button()==Qt::LeftButton){
        emit mousePressed(xRatio,yRatio,"left");
    }
    else if(ev->button()==Qt::RightButton){
        emit mousePressed(xRatio,yRatio,"right");
    }
    else if(ev->button()==Qt::MiddleButton){
        emit mousePressed(xRatio,yRatio,"middle");
    }

    setFocus();
    QLabel::mousePressEvent(ev);

}

void RemoteViewWidget::mouseReleaseEvent(QMouseEvent *ev)
{

    QPointF pos = ev->position();
    double xRatio=-1;
    double yRatio=-1;
    if(!positionToRatio(pos,xRatio,yRatio)) return;

    if(xRatio==-1 || yRatio==-1) return;

    if (ev->button() == Qt::LeftButton) {
        emit mouseReleased(xRatio, yRatio, "left");
    } else if (ev->button() == Qt::RightButton) {
        emit mouseReleased(xRatio, yRatio, "right");
    }
    else if(ev->button()==Qt::MiddleButton){
        emit mouseReleased(xRatio, yRatio, "middle");
    }

    QLabel::mouseReleaseEvent(ev);
}

void RemoteViewWidget::mouseMoveEvent(QMouseEvent *ev)
{
    //检查距离上次发送是否超过 40ms
    if(m_moveTimer.elapsed()<m_moveIntervalMs){
        return;
    }
    m_moveTimer.restart();

    QPointF pos=ev->position();
    double xRatio=-1;
    double yRatio=-1;
    if(!positionToRatio(pos,xRatio,yRatio)) return;

    if(xRatio==-1 || yRatio==-1) return;

    //发送信号
    emit mouseMove(xRatio,yRatio);

}

void RemoteViewWidget::wheelEvent(QWheelEvent *ev)
{

    QPointF angle=ev->angleDelta();
    int deltaY=angle.y();

    double xRatio=-1;
    double yRatio=-1;

    if(!this->positionToRatio(ev->position(),xRatio,yRatio)) {
        return;
    }
    if(xRatio==-1 || yRatio==-1) return;

    emit mouseWheel(xRatio,yRatio,deltaY);

}

void RemoteViewWidget::keyPressEvent(QKeyEvent *ev)
{
    //if(ev->isAutoRepeat()) return;

    emit keyEvent(ev->key(),int(ev->modifiers()),true,ev->text());
}

void RemoteViewWidget::keyReleaseEvent(QKeyEvent *ev)
{
    if(ev->isAutoRepeat()) return;

    emit keyEvent(ev->key(),int(ev->modifiers()),false,ev->text());
}

bool RemoteViewWidget::positionToRatio(const QPointF &pos, double &xRatio, double &yRatio)
{
    QPixmap pix = pixmap();
    if (pix.isNull()) {
        return false;
    }

    QSize pixSize = pix.size();         //image大小
    QSize labelSize = size();           //label大小

    int offsetX = (labelSize.width() - pixSize.width()) / 2;            //x轴偏移量
    int offsetY = (labelSize.height() - pixSize.height()) / 2;          //y轴偏移量

    double imageX = pos.x() - offsetX;      //image坐标x
    double imageY = pos.y() - offsetY;      //image坐标y
    
    // 点到黑边/空白区域，不发送鼠标事件
    if (imageX < 0 || imageY < 0 || imageX >= pixSize.width() || imageY >= pixSize.height()) {
        return false; 
    }

    xRatio = imageX / pixSize.width();
    yRatio = imageY / pixSize.height();

    return true;
}
