#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QElapsedTimer>

class RemoteSession;
class DiscoveryClient;
class FileTransferClient;
class FrameDecodeWorker;
class VideoFrameParser;
class UdpVideoReceiver;
class QThread;

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
    QByteArray m_pendingJpegData;


    //显示fps
    QElapsedTimer m_fpsTimer;
    int m_displayFrameCnt=0;
    quint64 m_jpegFramesReady = 0;
    quint64 m_pendingJpegOverwrites = 0;

    //video payload解析
    VideoFrameParser *m_videoFrameParser=nullptr;

    UdpVideoReceiver *m_udpVideoReceiver=nullptr;

private slots:
    //添加发现的远程设备
    void onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName);

    //连接server
    void onConnectBtnClicked();

    //将解码后的image显示到label中
    void onFrameReceived(const QImage &image);

    //接收解析后的 JPEG 帧
    void onJpegFrameReceived(const QByteArray &jpegData);
    
    //将解码的帧数据发送给label，防止帧积压
    void onFrameDecoded(const QImage &image);

    //解码失败
    void onFrameDecodeFailed();

    void on_sendFile_pushButton_clicked();
    void on_disconnect_pushButton_clicked();
};
#endif // MAINWINDOW_H
