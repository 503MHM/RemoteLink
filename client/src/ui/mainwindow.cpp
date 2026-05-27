#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "network/RemoteSession.h"
#include "network/DiscoveryClient.h"
#include "protocol/NetworkConstants.h"
#include "file/FileTransferClient.h"
#include "decode/FrameDecodeWorker.h"

#include <QTimer>
#include <QPixmap>
#include <QFileDialog>
#include <QDir>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_session(new RemoteSession(this))
    , m_fileTransferClient(new FileTransferClient(this))
{
    ui->setupUi(this);
    this->setWindowTitle("Client");
    ui->main_label->setAlignment(Qt::AlignCenter);
    
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

    //状态提示
    connect(m_session,&RemoteSession::connected,this,[this](){
        ui->status_label->setText("已连接");
        ui->sendFile_btn->setEnabled(true);
    });
    connect(m_session, &RemoteSession::disconnected,this, [this]() {
        ui->status_label->setText("已断开");
        m_fileTransferClient->cancel();         //断连后让子线程停止继续读文件
        ui->sendFile_btn->setEnabled(true);
    });
    connect(m_session, &RemoteSession::errorOccurred,this, [this](const QString &message) {
        ui->status_label->setText("连接错误：" + message);
    });


    //接收照片
    //connect(m_session,&RemoteSession::frameReceived,this,&MainWindow::onFrameReceived);

    //发送鼠标的坐标
    connect(ui->main_label,&RemoteViewWidget::mouseClicked,this,[=](double xRatio, double yRatio, const QString &btn){
        m_session->sendMouseClick(xRatio,yRatio,btn);
    });

    //发送鼠标移动
    connect(ui->main_label,&RemoteViewWidget::mouseMove,this,[=](double xRatio, double yRatio){
        m_session->sendMouseMove(xRatio,yRatio);
    });

    //发送鼠标滚轮
    connect(ui->main_label,&RemoteViewWidget::mouseWheel,this,[=](double xRatio, double yRatio, int deltaY){
        m_session->sendMouseWheel(xRatio,yRatio,deltaY);
    });

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
        ui->sendFile_btn->setEnabled(true);
    });

    connect(m_fileTransferClient, &FileTransferClient::errorOccurred,
        this, [this](const QString &message) {
            ui->status_label->setText(message);
            ui->sendFile_btn->setEnabled(true);
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
        m_displayFrameCnt=0;
        m_fpsTimer.restart();
    }
}


void MainWindow::on_sendFile_btn_clicked()
{
    ui->file_progressBar->setValue(0);

    QString filePath=QFileDialog::getOpenFileName(this,"选择要发送的文件",QDir::rootPath(),"all(*.*)");
    if(filePath.isEmpty()) return;
    ui->sendFile_btn->setEnabled(false);
    
    m_fileTransferClient->sendFile(filePath,m_session);
}

void MainWindow::onFrameDataReceived(const QByteArray &frameData)
{
    m_receivedFrameCnt++;
    if(m_receivedTimer.elapsed()>=1000){
        qDebug()<<"[client] received fps ="<<m_receivedFrameCnt;
        m_receivedFrameCnt=0;
        m_receivedTimer.restart();
    }


    //丢弃旧帧，只保留最新帧
    if(m_decodeBusy){
        m_pendingFrameData=frameData;
        return;
    }

    m_decodeBusy=true;
    QMetaObject::invokeMethod(m_frameDecodeWorker,"decodeFrame",Qt::ConnectionType::QueuedConnection,frameData);

}

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
