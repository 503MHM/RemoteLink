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
    bool positionToRatio(const QPointF &pos,double &xRatio,double &yRatio);

private:
    QElapsedTimer m_moveTimer;
    int m_moveIntervalMs=16;
};