#ifndef PROBEDEVICE_H
#define PROBEDEVICE_H

#include <QObject>
#include <QTimer>
#include "commheader.h"
#include "commstuct.h"
class ProbeDevice : public QObject
{
    Q_OBJECT
public:
    explicit ProbeDevice(QObject *parent = nullptr);

public:
    void Start();
    void Stop();
    bool TestDev(QString &deviceName, int &nBufType);
    //static EuFW_Type fwType();

protected:
    void listVideo();
    void listFmt(int fd, int nBufType);
    void listFrameSizes(int fd, uint32_t pixelformat, int nBufType);
    void listFrameIntervals(int fd, uint32_t pixfmt, uint32_t width, uint32_t height);

signals:
    void discovery(QString dev);
    void operateLog(int code, QString strMsg);

protected:
    QTimer *m_timer = nullptr;
};

#endif // PROBEDEVICE_H
