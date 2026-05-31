#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QElapsedTimer>
#include <QTimer>
#include <QTime>

class RemoteServer;
class DiscoveryResponder;
class ServerWorker;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    QString localIPV4Text();

public slots:
    void on_startORStop_pushButton_clicked();

private:
    Ui::MainWindow *ui;


    QThread *m_serverThread=nullptr;
    ServerWorker *m_serverWorker=nullptr;

    quint16 m_tcpPort=45454;
    bool m_serverListening=false;

    DiscoveryResponder *m_discoveryResponder=nullptr;

    QElapsedTimer m_clientConnectTimer;
    QTimer *m_clientTimeTimer=nullptr;

    //QTime nowTime;

    QString getNowTime();
};
#endif // MAINWINDOW_H
