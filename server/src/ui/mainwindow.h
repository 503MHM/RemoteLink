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

    //获取本机IPv4地址
    QString localIPV4Text();

public slots:
    void on_startORStop_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    // 服务器工作线程，负责TCP通信和协议处理
    QThread *m_serverThread=nullptr;
    ServerWorker *m_serverWorker=nullptr;

    quint16 m_tcpPort=45454;
    // 服务器是否开启服务的标志
    bool m_serverStarted=false;

    // 局域网设备发现响应器，用于广播和响应发现请求
    DiscoveryResponder *m_discoveryResponder=nullptr;

    // 客户端连接计时器，记录连接时长
    QElapsedTimer m_clientConnectTimer;
    QTimer *m_clientTimeTimer=nullptr;

    QString getNowTime();
};
#endif // MAINWINDOW_H
