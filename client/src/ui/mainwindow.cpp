#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteSession.h"
#include "network/DiscoveryClient.h"
#include "protocol/NetworkConstants.h"

#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_session(new RemoteSession(this))
{
    ui->setupUi(this);
    this->setWindowTitle("Client");

    // QTimer::singleShot(500, this, [this]() {
    //     m_session->connectToServer(QStringLiteral("127.0.0.1"), 45454);
    // });

    //
    m_discoveryClient=new DiscoveryClient(this);
    if(m_discoveryClient->start(DiscoveryPort)){
        m_discoveryClient->search();
    }

    //udp搜索到设备后显示在窗口上
    connect(m_discoveryClient,&DiscoveryClient::serverFound,this,&MainWindow::onServerFound);

    //刷新按钮
    connect(ui->refresh_pushButton,&QPushButton::clicked,this,[=](){
        ui->device_listWidget->clear();
        ui->status_label->setText("正在搜索局域网设备...");
        m_discoveryClient->search();
    });

    //双击后通过tcp连接某一设备
    connect(ui->device_listWidget,&QListWidget::itemDoubleClicked,this,&MainWindow::onItemDoubleClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName){

    //检查是否有重复的设备
    for(int i=0;i<ui->device_listWidget->count();i++){
        QListWidgetItem *tempItem=ui->device_listWidget->item(i);
        if(ip==tempItem->data(Qt::UserRole).toString() && tcpPort==tempItem->data(Qt::UserRole+1).toUInt()){
            return;
        }
    }

    //把设备信息显示到QListWidget中
    auto *item=new QListWidgetItem;
    item->setText(deviceName + "  " + ip + ":" + QString::number(tcpPort));
    item->setData(Qt::UserRole,ip);
    item->setData(Qt::UserRole+1,tcpPort);

    ui->device_listWidget->addItem(item);
    ui->status_label->setText("已发现设备：" + QString::number(ui->device_listWidget->count()));
}



void MainWindow::onItemDoubleClicked(QListWidgetItem *item){
    QString ip=item->data(Qt::UserRole).toString();
    quint16 port=item->data(Qt::UserRole+1).toUInt();

    m_session->connectToServer(ip,port);
}