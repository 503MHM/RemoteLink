#pragma once

#include <QLabel>
#include <QElapsedTimer>


class RemoteViewWidget : public QLabel
{
    Q_OBJECT

public:
    RemoteViewWidget(QWidget *parent=nullptr);

protected:
    void mousePressEvent(QMouseEvent *ev)override;
    void mouseReleaseEvent(QMouseEvent *ev)override;
    void mouseMoveEvent(QMouseEvent *ev)override;
    void wheelEvent(QWheelEvent *ev)override;
    void keyPressEvent(QKeyEvent *ev)override;
    void keyReleaseEvent(QKeyEvent *ev)override;



signals:
    void mouseMove(double xRatio,double yRatio);
    void mouseWheel(double xRatio, double yRatio, int delta);
    void keyEvent(int key, int modifiers, bool pressed, QString text);
    void mousePressed(double xRatio,double yRatio,const QString &btn);
    void mouseReleased(double xRatio,double yRatio,const QString &btn);


private:
    /**
     * @brief 将控件坐标转换为相对于显示图像的比例坐标
     * 
     * 将鼠标在控件上的绝对位置转换为相对于图像尺寸的比例值(0.0-1.0)。
     * 自动处理图像在控件中居中显示时产生的偏移量。
     * 
     * @param pos 鼠标在控件上的位置坐标
     * @param xRatio 输出的x轴比例值(相对于图像宽度的比例,范围0.0-1.0)
     * @param yRatio 输出的y轴比例值(相对于图像高度的比例,范围0.0-1.0)
     * @return true 转换成功,比例值已写入xRatio和yRatio，false 转换失败(无图像或点击位置在图像区域外)
     */
    bool positionToRatio(const QPointF &pos,double &xRatio,double &yRatio);

private:
    QElapsedTimer m_moveTimer;
    int m_moveIntervalMs=16;
};