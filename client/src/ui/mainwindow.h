#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>

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

private slots:
    void onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName);

    void onItemDoubleClicked(QListWidgetItem *item);
    void onFrameReceived(const QImage &image);
    void on_sendFile_btn_clicked();
};
#endif // MAINWINDOW_H
