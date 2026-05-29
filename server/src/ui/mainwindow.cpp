#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteServer.h"
#include "network/DiscoveryResponder.h"
#include "network/ServerWorker.h"

#include "../../common/protocol/NetworkConstants.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("server");
    

    //创建server子线程
    m_serverThread=new QThread(this);
    m_serverWorker=new ServerWorker;
    m_serverWorker->moveToThread(m_serverThread);

    connect(m_serverThread,&QThread::finished,m_serverWorker,&QObject::deleteLater);

    connect(m_serverWorker,&ServerWorker::started,this,[this](bool ok){
        //ui->statusbar->addWidget(new QLabel("server started"));
    });

    connect(m_serverWorker,&ServerWorker::errorOccurred,this,[this](const QString &message){
        
    });

    m_serverThread->start();

    //m_serverWorker 对象在“合适的时间”调用它自己的 start 函数
    QMetaObject::invokeMethod(m_serverWorker,
                              "start",
                              Qt::QueuedConnection,
                              Q_ARG(quint16,TcpPort));


    m_discoveryResponder=new DiscoveryResponder(this);
    m_discoveryResponder->start(DiscoveryPort,TcpPort);
}

MainWindow::~MainWindow()
{

    if(m_serverWorker){
        QMetaObject::invokeMethod(m_serverWorker,
                                  "stop",
                                  Qt::QueuedConnection);
    }

    if(m_serverThread){
        m_serverThread->quit();
        m_serverThread->wait();
    }

    delete ui;
}
