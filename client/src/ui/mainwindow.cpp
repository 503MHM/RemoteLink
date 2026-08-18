#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteSession.h"
#include "network/DiscoveryClient.h"
#include "protocol/NetworkConstants.h"
#include "file/FileTransferClient.h"
#include "decode/FrameDecodeWorker.h"
#include "protocol/VideoFramePayload.h"

#include <QTimer>
#include <QPixmap>
#include <QFileDialog>
#include <QDir>
#include <QThread>
#include <QVariantMap>
#include <QApplication>
#include <QFile>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_session(new RemoteSession(this))
    , m_fileTransferClient(new FileTransferClient(this))
{
    ui->setupUi(this);
    this->setWindowTitle("Client");
    this->setWindowIcon(QIcon(":/icon/icons/windowIcon.png"));

    QFile styleFile(":/styles/client_style.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    ui->statusIcon_label->setPixmap(QPixmap(":/icon/icons/disconnected.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->main_label->setAlignment(Qt::AlignCenter);
    ui->main_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->main_label->setMinimumSize(1, 1);
    
    m_fpsTimer.start();
    m_decodeFpsTimer.start();
    m_receivedTimer.start();


    //创建图片接码子线程
    m_frameDecodeThread=new QThread(this);
    m_frameDecodeWorker=new FrameDecodeWorker;
    m_frameDecodeWorker->moveToThread(m_frameDecodeThread);

    connect(m_session,&RemoteSession::frameDataReceived,this,&MainWindow::onFrameDataReceived);
    connect(m_frameDecodeWorker,&FrameDecodeWorker::frameDecoded,this,&MainWindow::onFrameDecoded);
    connect(m_frameDecodeThread,&QThread::finished,m_frameDecodeWorker, &QObject::deleteLater);
    connect(m_frameDecodeWorker,&FrameDecodeWorker::decodeFailed,this,&MainWindow::onFrameDecodeFailed);
    m_frameDecodeThread->start();


    //开启udp扫描设备
    m_discoveryClient=new DiscoveryClient(this);
    if(m_discoveryClient->start(RemoteLink::DiscoveryPort)){
        m_discoveryClient->search();
    }

    //udp搜索到设备后显示在窗口上
    connect(m_discoveryClient,&DiscoveryClient::serverFound,this,&MainWindow::onServerFound);

    //刷新按钮
    connect(ui->refresh_pushButton,&QPushButton::clicked,this,[=](){
        ui->device_comboBox->clear();
        ui->status_label->setText("正在搜索局域网设备...");
        m_discoveryClient->search();
    });

    //点击连接按钮通过tcp连接某一设备
    connect(ui->connect_pushButton,&QPushButton::clicked,this,&MainWindow::onConnectBtnClicked);

    //状态提示
    connect(m_session,&RemoteSession::connected,this,[this](){
        ui->status_label->setText("已连接："+ui->device_comboBox->currentText());
        ui->statusIcon_label->setPixmap(QPixmap(":/icon/icons/connected.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->sendFile_pushButton->setEnabled(true);
        ui->disconnect_pushButton->setEnabled(true);
        ui->connect_pushButton->setEnabled(false);
    });
    connect(m_session, &RemoteSession::disconnected,this, [this]() {
        ui->status_label->setText("已断开");
        m_fileTransferClient->cancel();         //断连后让子线程停止继续读文件
        ui->sendFile_pushButton->setEnabled(true);
        ui->disconnect_pushButton->setEnabled(false);
        ui->connect_pushButton->setEnabled(true);
        ui->statusIcon_label->setPixmap(QPixmap(":/icon/icons/disconnected.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    });
    connect(m_session, &RemoteSession::errorOccurred,this, [this](const QString &message) {
        ui->status_label->setText("连接错误：" + message);
    });



    //发送鼠标移动
    connect(ui->main_label,&RemoteViewWidget::mouseMove,m_session,&RemoteSession::sendMouseMove);

    //发送鼠标滚轮
    connect(ui->main_label,&RemoteViewWidget::mouseWheel,m_session,&RemoteSession::sendMouseWheel);

    //发送鼠标按下
    connect(ui->main_label,&RemoteViewWidget::mousePressed,m_session,&RemoteSession::sendMousePress);

    //发送鼠标抬起
    connect(ui->main_label,&RemoteViewWidget::mouseReleased,m_session,&RemoteSession::sendMouseRelease);

    //发送键盘信息
    connect(ui->main_label,&RemoteViewWidget::keyEvent,m_session,&RemoteSession::sendKeyEvent);

    //显示文件发送进度
    connect(m_fileTransferClient,&FileTransferClient::progressChanged,this,[=](qint64 sentBytes, qint64 totalBytes){
        if (totalBytes <= 0) {
            ui->file_progressBar->setValue(0);
            return;
        }
        
        ui->file_progressBar->setValue(static_cast<int>(sentBytes*100/totalBytes));
    });

    //完成后可再次选文件
    connect(m_fileTransferClient,&FileTransferClient::finished,this,[=](){
        ui->file_progressBar->setValue(100);
        ui->sendFile_pushButton->setEnabled(true);
    });

    connect(m_fileTransferClient, &FileTransferClient::errorOccurred,
        this, [this](const QString &message) {
            ui->status_label->setText(message);
            ui->sendFile_pushButton->setEnabled(true);
        });
}

MainWindow::~MainWindow()
{

    m_frameDecodeThread->quit();
    m_frameDecodeThread->wait();

    delete ui;
}



void MainWindow::onServerFound(const QString &ip, quint16 tcpPort, const QString &deviceName){

    //检查是否有重复的设备
    for(int i=0;i<ui->device_comboBox->count();i++){
        QVariantMap itemdata=ui->device_comboBox->itemData(i).toMap();
        if(itemdata.value("ip")==ip && itemdata.value("tcpPort")==tcpPort){
            return;
        }
    }


    QVariantMap data;
    data["ip"]=ip;
    data["tcpPort"]=tcpPort;
    data["deviceName"]=deviceName;

    ui->device_comboBox->addItem(QString("%1  %2:%3").arg(deviceName).arg(ip).arg(tcpPort),data);
    ui->status_label->setText("已发现设备："+QString::number(ui->device_comboBox->count()));
}


void MainWindow::onConnectBtnClicked(){
    
    QVariantMap data=ui->device_comboBox->currentData().toMap();
    if(data.isEmpty()){
        return;
    }
    
    
    QString ip=data.value("ip").toString();
    quint16 port=data.value("tcpPort").toUInt();

    m_session->connectToServer(ip,port);
}


void MainWindow::onFrameReceived(const QImage &image){
    auto pixmap = QPixmap::fromImage(image);

    ui->main_label->setPixmap(
    pixmap.scaled(ui->main_label->size(),
    Qt::KeepAspectRatio,
    Qt::SmoothTransformation)
    );

    m_displayFrameCnt++;

    if(m_fpsTimer.elapsed()>=1000){
        qDebug()<<"[client] display fps ="<<m_displayFrameCnt;
        ui->fps_label->setText(QString("fps: %1").arg(m_displayFrameCnt));
        
        m_displayFrameCnt=0;
        m_fpsTimer.restart();
        
    }
}



void MainWindow::onFrameDataReceived(const QByteArray &frameData)
{
    //解析视频帧
    RemoteLink::VideoFrameHeader vfh;
    QByteArray videoData;
    if(!RemoteLink::decodeVideoFramePayload(frameData,&vfh,&videoData))
    {
        return;
    }


    m_receivedFrameCnt++;
    if(m_receivedTimer.elapsed()>=1000){
        qDebug()<<"[client] received fps ="<<m_receivedFrameCnt;
        m_receivedFrameCnt=0;
        m_receivedTimer.restart();
    }


    //丢弃旧帧，只保留最新帧
    if(m_decodeBusy){
        m_pendingFrameData=videoData;
        return;
    }

    m_decodeBusy=true;
    QMetaObject::invokeMethod(m_frameDecodeWorker,"decodeFrame",Qt::ConnectionType::QueuedConnection,videoData);

}

//一帧解码完成
void MainWindow::onFrameDecoded(const QImage &image)
{

    m_decodeFrameCnt++;
    if(m_decodeFpsTimer.elapsed()>1000){
        qDebug()<< "[client] decoded fps ="<<m_decodeFrameCnt;
        m_decodeFrameCnt=0;
        m_decodeFpsTimer.restart();
    }


    onFrameReceived(image);

    if(!m_pendingFrameData.isEmpty()){
        //解码下一帧
        QByteArray nextFrame=std::move(m_pendingFrameData);
        m_pendingFrameData.clear();

        QMetaObject::invokeMethod(m_frameDecodeWorker,"decodeFrame",Qt::QueuedConnection,nextFrame);

        m_decodeBusy=true;
        return;
    }

    m_decodeBusy=false;
}


//解码失败释放busy
void MainWindow::onFrameDecodeFailed()
{


    if (!m_pendingFrameData.isEmpty()) {
        QByteArray nextFrame = std::move(m_pendingFrameData);
        m_pendingFrameData.clear();

        QMetaObject::invokeMethod(
            m_frameDecodeWorker,
            "decodeFrame",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, nextFrame)
        );

        m_decodeBusy = true;
        return;
    }

    m_decodeBusy = false;
}

void MainWindow::on_sendFile_pushButton_clicked()
{
    ui->file_progressBar->setValue(0);

    QString filePath=QFileDialog::getOpenFileName(this,"选择要发送的文件",QDir::rootPath(),"all(*.*)");
    if(filePath.isEmpty()) return;
    ui->sendFile_pushButton->setEnabled(false);
    
    m_fileTransferClient->sendFile(filePath,m_session);
}


void MainWindow::on_disconnect_pushButton_clicked()
{
    if(m_session->isConnected()){
        m_session->disConnectFromHost();
        ui->connect_pushButton->setEnabled(true);
    }
}

