#ifndef CAPTUREMPLANE_H
#define CAPTUREMPLANE_H

#include "capture.h"

class CaptureMPlane : public Capture
{
public:
    explicit CaptureMPlane(QObject *parent = nullptr);
    ~CaptureMPlane();

public:
    void startStream(int fd, StCamParam &param) override;
    void stopStream() override;
protected:
    void initBuf() override;
    void freeBuf() override;
    void run() override;

protected:
    int num_planes = 0;
    StMplaneBuffer *m_buffers = nullptr;
};

#endif // CAPTUREMPLANE_H
