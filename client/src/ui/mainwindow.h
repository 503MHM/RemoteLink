#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>

class RemoteSession;
class DiscoveryClient;

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

private slots:
    void onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName);

    void onItemDoubleClicked(QListWidgetItem *item);
    void onFrameReceived(const QImage &image);
};
#endif // MAINWINDOW_H
