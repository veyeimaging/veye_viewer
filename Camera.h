#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QThread>
#include "Capture.h"
#include "CodeMap.h"
#include "CommHeader.h"
#include "CommStuct.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct sensor_regs
{
    uint16_t reg;
    uint32_t data;
};

#pragma pack(push, 1)
struct preread_regs
{
    uint8_t pre_head;
    uint16_t reg;
    uint8_t xor_check;
};
struct write_regs
{
    uint8_t head;
    uint16_t reg;
    uint32_t data;
    uint8_t xor_check;
};
#pragma pack(pop)

class Camera : public QObject
{
    Q_OBJECT
public:
    explicit Camera(CommDevCfg devCfg, QObject *parent = nullptr);
    virtual ~Camera();

public:
    bool OpenDevice(StCamParam &param);
    void CloseDevice();
    // 多相机改造：针对本通道 Capture 的显示/存图控制（同线程直接转发，等价原信号调用）
    void setShow(bool bShow);
    void saveImg(QString strPath, int count);
    bool IsOpen() const { return m_capture != nullptr; }

signals:
    void sndOptLog(int ret, QString strInfo, bool show = true);
    void sndCmdRet(const StCmdRet &ret);

public:
    void runSystemCmd(QString strCmd);
    void asynReadData(StI2CCmd cmd);
    void asynWriteDate(StI2CCmd cmd);
    void syncReadData(StI2CCmd &cmd);

protected:
    int readData(StI2CCmd &cmd);
    int writeData(StI2CCmd &cmd);

protected:
    void cmdSleep(int n, bool bReal = true);
    void cmdThread();

protected:
    // 线程停止标志必须原子可见，且线程句柄要保存以便析构时 join，
    // 否则 cmdThread 会在对象释放后继续访问 m_mtx/m_listCmd 并在已析构的 QObject 上 emit。
    std::atomic<bool> m_bThread{true};
    std::thread m_threadCmd;

protected:
    std::list<StI2CCmd> m_listCmd;
    std::mutex m_mtx;
    std::mutex m_mtxCmd;

protected:
    std::unique_ptr<Capture> m_capture;
    int m_fd = -1;
    QObject *m_mainWidget;
    CommDevCfg m_devCfg;
    std::atomic<int> m_refCount = 0;
};

#endif // CAMERA_H
