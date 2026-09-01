#include "UdpVideoSender.h"
#include "protocol/UdpVideoPacket.h"

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

void UdpVideoSender::setFragmentPayloadSize(qsizetype fragmentPayloadSize)
{
    if(fragmentPayloadSize<=0)
    {
        return;
    }
    m_fragmentPayloadSize=fragmentPayloadSize;
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
    if(!isReady())
    {
        emit errorOccurred("[UdpVideoSender] is not ready");
        return false;
    }

    if(videoPayload.isEmpty())
    {
        emit errorOccurred("[UdpVideoSender] video payload is empty");
        return false;
    }

    const qsizetype packetCount64=(videoPayload.size()+m_fragmentPayloadSize-1)/m_fragmentPayloadSize;
    if(packetCount64<=0 || packetCount64>std::numeric_limits<quint16>::max())
    {
        emit errorOccurred("[UdpVideoSender] packet count out of range");

        return false;
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
            return false;
        }

        qint64 written=m_socket.writeDatagram(fragment,m_targetAddress,m_targetPort);
        if(written!=fragment.size())
        {
            emit errorOccurred("[UdpVideoSender] write datagram failed");
            return false;
        }
    }

    return true;
}
