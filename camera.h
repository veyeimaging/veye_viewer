#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include "capture.h"
#include "commstuct.h"
#include <memory>

class Camera : public QObject
{
    Q_OBJECT
public:
    explicit Camera(QObject *parent = nullptr);
    virtual ~Camera();

public:
    bool OpenDevice(StCamParam &param);
    void CloseDevice();
protected:
    std::unique_ptr<Capture> m_capture;
    int m_fd = -1;
    QObject *m_mainWidget;
};

#endif // CAMERA_H
