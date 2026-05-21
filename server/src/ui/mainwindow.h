#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class RemoteServer;
class DiscoveryResponder;

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
    RemoteServer *m_server = nullptr;

    DiscoveryResponder *m_discoveryResponder;
};
#endif // MAINWINDOW_H
