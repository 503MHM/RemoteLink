#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

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

private:
    Ui::MainWindow *ui;


    QThread *m_serverThread=nullptr;
    ServerWorker *m_serverWorker=nullptr;

    DiscoveryResponder *m_discoveryResponder;
};
#endif // MAINWINDOW_H
