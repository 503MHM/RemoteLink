#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteServer.h"
#include "network/DiscoveryResponder.h"
#include "network/ServerWorker.h"
#include "../../common/protocol/NetworkConstants.h"

#include <QHostInfo>
#include <QNetworkInterface>
#include <QApplication>
#include <QFile>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("server");
    this->setWindowIcon(QIcon(":/icon/icons/windowIcon.png"));

    QFile styleFile(":/styles/server_style.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
    

    //创建server子线程
    m_serverThread=new QThread(this);
    m_serverWorker=new ServerWorker;
    m_serverWorker->moveToThread(m_serverThread);

    connect(m_serverThread,&QThread::finished,m_serverWorker,&QObject::deleteLater);

    connect(m_serverWorker,&ServerWorker::started,this,[this](bool ok){
        m_serverStarted=ok;
        ui->startORStop_pushButton->setText(ok? "停止服务" : "启动服务");

        ui->log_textEdit->append(getNowTime()+" 已启动服务");
    });

    connect(m_serverWorker,&ServerWorker::stoped,this,[this](){
        m_serverStarted=false;
        ui->startORStop_pushButton->setText("启动服务");

        ui->connectStatus_label->setText("未连接");

        if (m_clientTimeTimer) {
            m_clientTimeTimer->stop();
        }

        ui->serverTime_label->setText("--:--:--");

        ui->log_textEdit->append(getNowTime()+" 已停止服务");
    });

    connect(m_serverWorker,&ServerWorker::errorOccurred,this,[this](const QString &message){
        ui->log_textEdit->append(getNowTime()+" "+message);
    });

    m_serverThread->start();


    //获取用户输入的要监听的端口
    connect(ui->tcpPort_lineEdit,&QLineEdit::editingFinished,this,[=](){
        quint16 port=ui->tcpPort_lineEdit->text().toUInt();
        if(!port) return;
        m_tcpPort=port;

        ui->log_textEdit->append(getNowTime()+" TCP服务已监听  "+localIPV4Text()+QString(":%1").arg(m_tcpPort));
    });

    //设置本机ip地址和本机名称
    ui->deviceName_label->setText(QHostInfo::localHostName());
    ui->ip_label->setText(localIPV4Text());

    //设置运行状态
    connect(m_serverWorker,&ServerWorker::clientConnected,this,[this](QString clientIp, quint16 clientPort, QString localIp, quint16 localPort){
        ui->connectStatus_label->setText(QString("已连接 %1:%2").arg(clientIp).arg(clientPort));
        ui->ip_label->setText(localIp);

        m_clientConnectTimer.restart();
        m_clientTimeTimer->start(1000);
        ui->serverTime_label->setText("00:00:00");

        ui->disconnect_pushButton->setEnabled(true);

        ui->log_textEdit->append(getNowTime()+QString(" 已连接 %1:%2").arg(clientIp).arg(clientPort));
    });
    connect(m_serverWorker,&ServerWorker::clientDisconnected,this,[=](){
        ui->connectStatus_label->setText("未连接");

        if(m_clientTimeTimer){
            m_clientTimeTimer->stop();
        }
        
        ui->disconnect_pushButton->setEnabled(false);

        ui->log_textEdit->append(getNowTime()+" 已断开连接,连接时长:"+ui->serverTime_label->text());
        ui->serverTime_label->setText("--:--:--");
    });

    //设置运行时间
    m_clientTimeTimer = new QTimer(this);

    connect(m_clientTimeTimer, &QTimer::timeout, this, [this]() {
        qint64 seconds = m_clientConnectTimer.elapsed() / 1000;

        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        int s = seconds % 60;

        ui->serverTime_label->setText(
            QString("%1:%2:%3")
                .arg(h, 2, 10, QChar('0'))
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0'))
        );
    });


    //设置文件传输相关
    connect(m_serverWorker,&ServerWorker::receiveStarted,this,[=](const QString &fileName, quint64 totalBytes){
        ui->receiverFile_label->setText("正在接收: "+fileName);
        ui->file_progressBar->setValue(0);

        ui->log_textEdit->append(getNowTime()+" 正在接收: "+fileName);
    });
    connect(m_serverWorker,&ServerWorker::receiveProgress,this,[=](quint64 receivedBytes, quint64 totalBytes){
        ui->file_progressBar->setValue(receivedBytes*100/totalBytes);
    });
    connect(m_serverWorker,&ServerWorker::receiveFinished,this,[=](bool ok,QString filePath){
        if(ok){
            ui->receiverFile_label->setText("文件接收完毕");
            ui->log_textEdit->append(getNowTime()+"文件接收完毕,保存在: "+filePath);
        }
        else {
            ui->receiverFile_label->setText("文件接收不完整");
            ui->log_textEdit->append(getNowTime()+"文件接收不完整");
        }
    });


    //连接断开连接按钮
    connect(ui->disconnect_pushButton,&QPushButton::clicked,this,[=](){
        QMetaObject::invokeMethod(m_serverWorker,"disconnectClient",Qt::QueuedConnection);
    });
}

MainWindow::~MainWindow()
{   
    if(m_discoveryResponder){
        m_discoveryResponder->stop();
    }

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

void MainWindow::on_startORStop_pushButton_clicked()
{
    if(m_serverStarted){

        //关闭UDP服务
        if(m_discoveryResponder){
            m_discoveryResponder->stop();
            m_discoveryResponder->deleteLater();
            m_discoveryResponder=nullptr;
        }

        m_serverStarted=false;
        ui->startORStop_pushButton->setText("启动服务");

        QMetaObject::invokeMethod(m_serverWorker,"stop",Qt::QueuedConnection);
    }

    else{
        if(!m_discoveryResponder){
            m_discoveryResponder=new DiscoveryResponder(this);
        }

        if(!m_discoveryResponder->start(DiscoveryPort,m_tcpPort)){
            ui->statusbar->addWidget(new QLabel("服务启动失败"));
            return;
        }


        QMetaObject::invokeMethod(m_serverWorker,"start",Qt::QueuedConnection,Q_ARG(quint16,m_tcpPort));
    }
}



QString MainWindow::localIPV4Text()
{

    QStringList ips;

    //获取所有网卡
    auto interfaces=QNetworkInterface::allInterfaces();
    for(QNetworkInterface &iface:interfaces){
        if(!(iface.flags()&QNetworkInterface::IsUp)||
           !(iface.flags()&QNetworkInterface::IsRunning)||
            (iface.flags()&QNetworkInterface::IsLoopBack)){
                continue;
        }

        //遍历每个网卡上可用的IPV4地址
        for(const QNetworkAddressEntry &entry:iface.addressEntries()){
            QHostAddress ip=entry.ip();

            //不是IPV4不要
            if(ip.protocol()!=QAbstractSocket::IPv4Protocol){
                continue;
            }

            QString ipText=ip.toString();

            if(ipText.startsWith("169.254.")) continue;

            ips<<ipText;
        }

    }

    return ips.isEmpty() ? "-" : ips.join(" / ");
}

QString MainWindow::getNowTime()
{
    return QTime::currentTime().toString("[hh:mm:ss]");
}
