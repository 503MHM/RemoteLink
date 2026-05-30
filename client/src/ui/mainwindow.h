#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QElapsedTimer>

class RemoteSession;
class DiscoveryClient;
class FileTransferClient;
class FrameDecodeWorker;

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

private:
    Ui::MainWindow *ui;
    RemoteSession *m_session = nullptr;
    DiscoveryClient *m_discoveryClient=nullptr;
    FileTransferClient *m_fileTransferClient=nullptr;

    QThread *m_frameDecodeThread=nullptr;
    FrameDecodeWorker *m_frameDecodeWorker=nullptr;

    bool m_decodeBusy=false;
    QByteArray m_pendingFrameData;


    //显示fps
    QElapsedTimer m_fpsTimer;
    int m_displayFrameCnt=0;

    //解码fps
    QElapsedTimer m_decodeFpsTimer;
    int m_decodeFrameCnt=0;

    //收到的数据帧的数量
    QElapsedTimer m_receivedTimer;
    int m_receivedFrameCnt=0;


private slots:
    void onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName);

    void onConnectBtnClicked();
    void onFrameReceived(const QImage &image);

    void onFrameDataReceived(const QByteArray &frameData);
    void onFrameDecoded(const QImage &image);
    void onFrameDecodeFailed();
    void on_sendFile_pushButton_clicked();
    void on_disconnect_pushButton_clicked();
};
#endif // MAINWINDOW_H
