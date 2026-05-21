#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteServer.h"
#include "network/DiscoveryResponder.h"

#include "../../common/protocol/NetworkConstants.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_server(new RemoteServer(this))
{
    ui->setupUi(this);
    this->setWindowTitle("server");
    m_server->listen(TcpPort);

    m_discoveryResponder=new DiscoveryResponder(this);
    m_discoveryResponder->start(DiscoveryPort,TcpPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}
