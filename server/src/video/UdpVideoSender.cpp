#include "UdpVideoSender.h"
#include "protocol/UdpVideoPacket.h"
#include <QDebug>


UdpVideoSender::UdpVideoSender(QObject *parent)
    :QObject(parent)
{
}

UdpVideoSender::~UdpVideoSender()
{
    clearTarget();
}

void UdpVideoSender::setTarget(const QHostAddress &address, quint16 port)
{
    if(address.isNull() || port==0)
    {
        clearTarget();
        return;
    }

    m_targetAddress=address;
    m_targetPort=port;
}

void UdpVideoSender::clearTarget()
{
    m_targetAddress=QHostAddress::Null;
    m_targetPort=0;
}

void UdpVideoSender::setSessionId(quint32 sessionId)
{
    m_sessionId=sessionId;
}

quint32 UdpVideoSender::sessionId() const
{
    return m_sessionId;
}

bool UdpVideoSender::setFragmentPayloadSize(qsizetype fragmentPayloadSize)
{
    if(fragmentPayloadSize<=0 || 
        fragmentPayloadSize > RemoteLink::MaxUdpVideoFragmentPayloadSize)
    {
        return  false;
    }
    m_fragmentPayloadSize=fragmentPayloadSize;
    return true;
}

qsizetype UdpVideoSender::fragmentPayloadSize() const
{
    return m_fragmentPayloadSize;
}

bool UdpVideoSender::isReady() const
{
    return !m_targetAddress.isNull() &&
           m_targetPort!=0 &&
           m_sessionId!=0 && 
           m_fragmentPayloadSize > 0;
}

bool UdpVideoSender::sendFrame(quint32 frameId, const QByteArray &videoPayload)
{
    quint64 sentDatagrams = 0;
    quint64 sentBytes = 0;
    auto finish = [this, &sentDatagrams, &sentBytes](bool success) {
        recordSendStats(success, sentDatagrams, sentBytes);
        return success;
    };

    if(!isReady())
    {
        emit errorOccurred("[UdpVideoSender] is not ready");
        return finish(false);
    }

    if(videoPayload.isEmpty())
    {
        emit errorOccurred("[UdpVideoSender] video payload is empty");
        return finish(false);
    }

    if(videoPayload.size()>RemoteLink::MaxUdpVideoFramePayloadSize || 
        m_fragmentPayloadSize <= 0 ||
        m_fragmentPayloadSize > RemoteLink::MaxUdpVideoFragmentPayloadSize)
    {
        emit errorOccurred("[UdpVideoSender] video payload or fragment size out of range");
        return finish(false);
    }

    const qsizetype packetCount64=(videoPayload.size()+m_fragmentPayloadSize-1)/m_fragmentPayloadSize;
    if(packetCount64<=0 || packetCount64 > RemoteLink::MaxUdpVideoPacketCount)
    {
        emit errorOccurred("[UdpVideoSender] packet count out of range");

        return finish(false);
    }
    const quint16 packetCount=static_cast<quint16>(packetCount64);

    for(int packetIndex=0;packetIndex<packetCount;packetIndex++)
    {
        const qsizetype offset=static_cast<qsizetype>(m_fragmentPayloadSize*packetIndex);
        const qsizetype dataSize=qMin(m_fragmentPayloadSize,videoPayload.size()-offset);
        const QByteArray fragmentData=videoPayload.mid(offset,dataSize);

        RemoteLink::UdpVideoPacketHeader fragmentHeader;
        fragmentHeader.flags=0;
        fragmentHeader.frameId=frameId;
        fragmentHeader.packetCount=packetCount;
        fragmentHeader.packetIndex=packetIndex;
        fragmentHeader.payloadSize=static_cast<quint16>(fragmentData.size());
        fragmentHeader.sessionId=m_sessionId;

        QByteArray fragment = RemoteLink::encodeUdpVideoPacket(fragmentHeader,fragmentData);
        if(fragment.isEmpty())
        {
            emit errorOccurred("[UdpVideoSender] Fragment Encode Failed");
            return finish(false);
        }

        qint64 written=m_socket.writeDatagram(fragment,m_targetAddress,m_targetPort);
        if(written!=fragment.size())
        {
            const QString message = QStringLiteral(
                "[UdpVideoSender] writeDatagram failed: frameId=%1 fragment=%2/%3 "
                "written=%4 expected=%5 target=%6:%7 socketError=%8 errorString=%9")
                .arg(frameId)
                .arg(static_cast<uint>(packetIndex + 1))
                .arg(static_cast<uint>(packetCount))
                .arg(written)
                .arg(fragment.size())
                .arg(m_targetAddress.toString())
                .arg(m_targetPort)
                .arg(static_cast<int>(m_socket.error()))
                .arg(m_socket.errorString());
            emit errorOccurred(message);
            return finish(false);
        }

        ++sentDatagrams;
        sentBytes += static_cast<quint64>(written);
    }

    return finish(true);
}

void UdpVideoSender::recordSendStats(bool frameSent,
                                    quint64 datagrams,
                                    quint64 bytes)
{
    if (!m_statsTimer.isValid()) {
        m_statsTimer.start();
    }

    if (frameSent) {
        ++m_statsFramesSent;
    } else {
        ++m_statsFramesFailed;
    }
    m_statsDatagrams += datagrams;
    m_statsBytes += bytes;

    const qint64 intervalMs = m_statsTimer.elapsed();
    if (intervalMs < 1000) {
        return;
    }

    const double seconds = intervalMs / 1000.0;
    qInfo() << "[udp-tx] frames_per_sec=" << m_statsFramesSent / seconds
            << "datagrams_per_sec=" << m_statsDatagrams / seconds
            << "mib_per_sec=" << m_statsBytes / seconds / (1024.0 * 1024.0)
            << "fragment_payload_bytes=" << m_fragmentPayloadSize
            << "send_failures=" << m_statsFramesFailed;

    m_statsTimer.restart();
    m_statsFramesSent = 0;
    m_statsFramesFailed = 0;
    m_statsDatagrams = 0;
    m_statsBytes = 0;
}
