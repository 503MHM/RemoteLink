#pragma once

#include <QLabel>
#include <QElapsedTimer>


class RemoteViewWidget : public QLabel
{
    Q_OBJECT

public:
    RemoteViewWidget(QWidget *parent=nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *ev)override;
    void mouseMoveEvent(QMouseEvent *ev)override;
    void wheelEvent(QWheelEvent *ev)override;

signals:
    void mouseClicked(double xRatio,double yRatio,const QString &btn);
    void mouseMove(double xRatio,double yRatio);
    void mouseWheel(double xRatio, double yRatio, int delta);

private:
    bool positionToRatio(const QPointF &pos,double &xRatio,double &yRatio);

private:
    QElapsedTimer m_moveTimer;
    int m_moveIntervalMs=40;
};