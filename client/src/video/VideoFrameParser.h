#pragma once

#include <QByteArray>
#include <QObject>

class VideoFrameParser : public QObject
{
    Q_OBJECT

public:
    explicit VideoFrameParser(QObject *parent = nullptr);

public slots:
    void parseFramePayload(const QByteArray &payload);

signals:
    void jpegFrameReady(const QByteArray &jpegData);

};
