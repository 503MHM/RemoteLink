#include "UdpVideoReceiver.h"
#include "protocol/UdpVideoPacket.h"
#include <QHostAddress>
#include <QVariant>


UdpVideoReceiver::UdpVideoReceiver(QObject *parent)
    :QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &UdpVideoReceiver::onReadyRead);

    m_assemblyTimer.setSingleShot(true);
    m_assemblyTimer.setInterval(FrameAssemblyTimeoutMs);

    connect(&m_assemblyTimer, &QTimer::timeout, this, &UdpVideoReceiver::onAssemblyTimeout);
}

UdpVideoReceiver::~UdpVideoReceiver()
{
    stop();
}

bool UdpVideoReceiver::start(quint16 port)
{
    if(port==0)
    {
        emit errorOccurred(QStringLiteral("[UdpVideoReceiver] port 0 is not supported"));
        return false;
    }

    if(m_socket.state() == QAbstractSocket::BoundState)
    {
        if (m_socket.localPort() == port) {
            return true;
        }

        emit errorOccurred(QStringLiteral(
            "[UdpVideoReceiver] already bound to another port"));
        return false;
    }

    resetFrameAssembly();

    const bool ret=m_socket.bind(
        QHostAddress::AnyIPv4,
        port,
        QUdpSocket::DontShareAddress);
    if(!ret)
    {
        emit errorOccurred(QString("Failed to bind video receiving port: %1").arg(m_socket.errorString()));
        return false;
    }

    m_socket.setSocketOption(
        QAbstractSocket::ReceiveBufferSizeSocketOption,
        QVariant(4 * 1024 * 1024));

    return true;
}

void UdpVideoReceiver::stop()
{
    m_socket.close();
    m_assemblyTimer.stop();
    resetFrameAssembly();
}

bool UdpVideoReceiver::processDatagram(const QByteArray &datagram)
{
    RemoteLink::UdpVideoPacketHeader header;
    QByteArray fragmentData;

    if(!RemoteLink::decodeUdpVideoPacket(datagram,&header,&fragmentData))
    {
        return false;
    }

    if(m_currentSessionId==0 || header.sessionId!=m_currentSessionId)
    {
        return false;
    }

    if (header.packetCount==0 || header.packetCount>RemoteLink::MaxUdpVideoPacketCount)
    {
        return false;
    }

    if(header.packetIndex>=header.packetCount)
    {
        return false;
    }

    //状态机
    if(!m_hasCurrentFrame)
    {
        //当前没在拼帧
        if(!startNewFrame(header.frameId,header.packetCount))
        {
            return false;
        }
    }
    else if(header.frameId>m_currentFrameId)
    {
        //来了新帧丢弃旧的
        if(!startNewFrame(header.frameId,header.packetCount))
        {
            return false;
        }
    }
    else if(header.frameId<m_currentFrameId)
    {
        //迟到的分片丢弃
        return false;
    }
    else if(header.packetCount!=m_currentPacketCount)
    {
        //同一帧但是总数不对，数据异常重置
        resetFrameAssembly();
        return false;
    }

    if(!appendFragment(header.frameId,header.packetIndex,header.packetCount,fragmentData))
    {
        return false;
    }

    if(!isCurrentFrameComplete())
    {
        //一帧没拼齐
        return true;
    }

    //拼齐了组装发出
    QByteArray framePayload=assembleCurrentFrame();
    //重置
    resetFrameAssembly();
    if(framePayload.isEmpty())
    {
        return false;
    }

    //发送
    emit framePayloadReceived(framePayload);

    return true;
}

void UdpVideoReceiver::setSessionId(quint32 sessionId)
{
    if(m_currentSessionId==sessionId)
    {
        return;
    }

    m_currentSessionId=sessionId;
    resetFrameAssembly();
}

quint32 UdpVideoReceiver::sessionId() const
{
    return m_currentSessionId;
}

void UdpVideoReceiver::onReadyRead()
{
    while (m_socket.hasPendingDatagrams())
    {
        const qint64 datagramSize=m_socket.pendingDatagramSize();
        if (datagramSize<=0)
        {
            m_socket.readDatagram(nullptr,0);
            continue;
        }
        
        QByteArray datagram;
        datagram.resize(datagramSize);

        const qint64 receiverSize=m_socket.readDatagram(datagram.data(),datagramSize);
        if(receiverSize<0)
        {
            //读取失败
            errorOccurred(QStringLiteral("[UdpVideoReceiver] read datagram failed: %1")
            .arg(m_socket.errorString()));
            
            continue;
        }
        processDatagram(datagram);
    }
    
}

void UdpVideoReceiver::onAssemblyTimeout()
{
    resetFrameAssembly();
}

void UdpVideoReceiver::resetFrameAssembly()
{
    m_assemblyTimer.stop();
    m_currentFrameId=0;
    m_currentPacketCount=0;
    m_hasCurrentFrame=false;
    m_receivedFragments.clear();
    m_receivedPacketCount=0;
    m_currentFrameBytes=0;
}

bool UdpVideoReceiver::startNewFrame(quint32 frameId, quint16 packetCount)
{
    if(packetCount==0 || packetCount>RemoteLink::MaxUdpVideoPacketCount)
    {
        return false;
    }

    resetFrameAssembly();

    m_currentFrameId=frameId;
    m_currentPacketCount=packetCount;
    m_hasCurrentFrame=true;

    m_receivedFragments.resize(packetCount);
    m_receivedPacketCount=0;

    m_assemblyTimer.start();
    return true;
}

bool UdpVideoReceiver::appendFragment(quint32 frameId, quint16 packetIndex, quint16 packetCount, const QByteArray &fragmentData)
{
    if(!m_hasCurrentFrame || 
        frameId!=m_currentFrameId || 
        packetCount!=m_currentPacketCount ||
        packetIndex>=m_receivedFragments.size())
    {
        return false;
    }

    std::optional<QByteArray> &fragment=m_receivedFragments[packetIndex];
    if(fragment.has_value())
    {
        //重复分片忽略即可
        return true;
    }

    if(fragmentData.isEmpty() || 
        fragmentData.size()>RemoteLink::MaxUdpVideoFragmentPayloadSize ||
        m_currentFrameBytes > RemoteLink::MaxUdpVideoFramePayloadSize-fragmentData.size()    
    )
    {
        resetFrameAssembly();
        return false;
    }

    fragment=fragmentData;
    m_currentFrameBytes+=fragmentData.size();
    ++m_receivedPacketCount;

    m_assemblyTimer.start();
    return true;

}

bool UdpVideoReceiver::isCurrentFrameComplete() const
{
    if(!m_hasCurrentFrame ||
        m_receivedPacketCount != m_currentPacketCount || 
        m_receivedFragments.size() != m_currentPacketCount)
    {
        return false;
    }

    for(const auto &fragment:m_receivedFragments)
    {
        if(!fragment.has_value())
        {
            return false;
        }
    }

    return true;
}

QByteArray UdpVideoReceiver::assembleCurrentFrame() const
{
    if(!isCurrentFrameComplete())
    {
        return {};
    }

    qsizetype totalSize=0;
    for(const auto &fragment : m_receivedFragments)
    {
        totalSize+=fragment->size();
    }

    QByteArray framePayload;
    framePayload.reserve(totalSize);

    for(const auto &fragment : m_receivedFragments)
    {
        framePayload.append(*fragment);
    }

    return framePayload;

}
