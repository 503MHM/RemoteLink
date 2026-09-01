#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QString>
#include <QByteArray>


class UdpVideoSender : public QObject
{
    Q_OBJECT    
public:
    explicit UdpVideoSender(QObject *parent=nullptr);
    ~UdpVideoSender();

    void setTarget(const QHostAddress &address,quint16 port);
    void clearTarget();

    void setSessionId(quint32 sessionId);
    quint32 sessionId() const;

    void setFragmentPayloadSize(qsizetype fragmentPayloadSize);
    qsizetype fragmentPayloadSize() const;

    bool isReady() const;

    // videoPayload = VideoFrameHeader + JPEG bytes
    bool sendFrame(quint32 frameId, const QByteArray &videoPayload);

signals:
    void errorOccurred(const QString &message);

private:
    QUdpSocket m_socket;
    QHostAddress m_targetAddress;
    quint16 m_targetPort=0;

    quint32 m_sessionId=0;
    qsizetype m_fragmentPayloadSize=1200;
};
