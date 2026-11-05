#pragma once


#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <optional>

class UdpVideoReceiver : public QObject
{
    Q_OBJECT
public:
    UdpVideoReceiver(QObject *parent=nullptr);

    ~UdpVideoReceiver();

    //绑定视频接收端口并开始收包
    bool start(quint16 port);

    //停止收包并清空重组状态
    void stop();

    //处理单个UDP数据报：解码、分派、重组，拼齐则发出完整帧
    bool processDatagram(const QByteArray &datagram);

    //设置会话ID，用于过滤非本会话的包
    void setSessionId(quint32 sessionId);
    quint32 sessionId() const;

signals:
    //一帧完整重组的数据,videoFrameHeader+payload
    void framePayloadReceived(const QByteArray &framePayload);

    void errorOccurred(const QString &message);

private slots:

    //socket有数据到达时触发，循环读取所有待处理数据报
    void onReadyRead();
    //拼帧超时触发，放弃迟迟拼不齐的半帧
    void onAssemblyTimeout();
    void onStatsTimer();

private:
    //清空所有重组状态，回到空闲态
    void resetFrameAssembly();
    //开启新帧的重组：记录帧号、按分片数开槽位
    bool startNewFrame(quint32 frameId, quint16 packetCount);
    //把一个分片存入对应槽位（含校验和去重）
    bool appendFragment(quint32 frameId, quint16 packetIndex, quint16 packetCount, const QByteArray &fragmentData);
    //判断当前帧分片是否已收齐
    bool isCurrentFrameComplete() const;
    //按分片序号顺序拼接出完整帧payload
    QByteArray assembleCurrentFrame() const;

private:
    //拼帧超时时间：超过该时长未拼齐则放弃当前帧
    static constexpr int FrameAssemblyTimeoutMs=200;

    QUdpSocket m_socket;
    
    QTimer m_assemblyTimer;             //拼帧超时定时器（单次触发，每帧开始时启动）
    QTimer m_statsTimer;

    quint32 m_currentSessionId=0;       //当前会话ID，用于过滤非本会话的包
    
    quint32 m_currentFrameId=0;         //正在重组的帧号
    quint16 m_currentPacketCount=0;     //当前帧应有的分片总数
    
    bool m_hasCurrentFrame = false;     //是否正在拼帧（重组状态机的状态标志）
    
    QVector<std::optional<QByteArray>> m_receivedFragments;     //已收到的分片，下标=packetIndex，nullopt表示未收到
    
    quint16 m_receivedPacketCount=0;    //已收到的去重分片数量

    qsizetype m_currentFrameBytes = 0;

    quint64 m_statsDatagrams = 0;
    quint64 m_statsBytes = 0;
    quint64 m_statsAcceptedFragments = 0;
    quint64 m_statsCompletedFrames = 0;
    quint64 m_statsReplacedFrames = 0;
    quint64 m_statsTimedOutFrames = 0;
    quint64 m_statsMissingFragments = 0;
    quint64 m_statsAbandonedExpectedFragments = 0;
    quint64 m_statsRejectedDatagrams = 0;
    quint64 m_statsStaleFragments = 0;

};
