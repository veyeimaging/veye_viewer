#ifndef CAPTURESPLANE_H
#define CAPTURESPLANE_H

#include <QObject>
#include "capture.h"

class CaptureSPlane : public Capture
{
    Q_OBJECT
public:
    explicit CaptureSPlane(QObject *parent = nullptr);

public:
    void startStream(int fd, StCamParam &param) override;
    void stopStream() override;

protected:
    void initBuf() override;
    void freeBuf() override;
    void run() override;

protected:
    StSplaneBuffer *m_buffers = nullptr;
};

#endif // CAPTURESPLANE_H
