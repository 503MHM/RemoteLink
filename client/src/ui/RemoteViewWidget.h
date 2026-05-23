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

signals:
    void mouseClicked(double xRatio,double yRatio,const QString &btn);
    void mouseMove(double xRatio,double yRatio);

private:
    bool positionToRatio(const QPointF &pos,double &xRatio,double &yRatio);

private:
    QElapsedTimer m_moveTimer;
    int m_moveIntervalMs=40;
};