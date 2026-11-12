#include "Camera.h"
#include <QProcess>
#include "CaptureMPlane.h"
#include "CaptureSPlane.h"
#include "MainWidget.h"
#include "I2CLock.h"
#include "QxHelp.h"

#define WRITE_HEAD 0xAB
#define PRE_READ_HEAD 0xBC
#define POST_READ_HEAD 0xAC
#define DIR_READ_HEAD 0xDE

static uint8_t xor8(const uint8_t *data, uint32_t len)
{
    if (data == nullptr || len == 0) {
        return 0;
    }
    uint8_t checksum = 0;
    const uint8_t *p = data;
    while (len--) {
        checksum ^= *p++;
    }
    return checksum;
}

static int gxcam_readl_reg(int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
{
    struct preread_regs regs;
    uint8_t checksum = 0;
    uint8_t buf[3] = {POST_READ_HEAD,
                      static_cast<uint8_t>(reg >> 8),
                      static_cast<uint8_t>(reg & 0xff)};
    uint8_t bufout[8] = {0};

    regs.pre_head = PRE_READ_HEAD;
    regs.reg = htons(reg);
    regs.xor_check = xor8((uint8_t *) &regs, 3);

    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msg = {
        .addr = i2c_addr,
        .flags = 0,
        .len = sizeof(regs),
        .buf = (uint8_t *) &regs,
    };

    msgset.msgs = &msg;
    msgset.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &msgset) != static_cast<int>(msgset.nmsgs)) {
        printf("Write i2c err \n");
        return -1;
    }

    usleep(20000);

    struct i2c_msg msgs[2] = {
        {
            .addr = i2c_addr,
            .flags = 0,
            .len = 3,
            .buf = buf,
        },
        {
            .addr = i2c_addr,
            .flags = I2C_M_RD,
            .len = 6,
            .buf = bufout,
        },
    };
    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &msgset) != static_cast<int>(msgset.nmsgs)) {
        printf("Read i2c err\n");
        return -1;
    }

    checksum = xor8(bufout, 5);
    if (checksum != bufout[5]) {
        printf("%s: Read register 0x%02x checksum error\n", __func__, reg);
        return -EIO;
    }
    *val = ntohl(*(int32_t *) bufout);
    return 0;
}

static int gxcam_writel_reg(int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
{
    struct write_regs wr;

    wr.head = WRITE_HEAD;
    wr.reg = htons(reg);
    wr.data = htonl(val);
    wr.xor_check = xor8((uint8_t *) &wr, sizeof(wr) - 1);
    /*	return write(fd, &wr,sizeof(wr));*/
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msg = {
        .addr = i2c_addr,
        .flags = 0,
        .len = sizeof(wr),
        .buf = (uint8_t *) &wr,
    };

    msgset.msgs = &msg;
    msgset.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &msgset) != static_cast<int>(msgset.nmsgs)) {
        printf("Write i2c err \n");
        return -1;
    }
    return 0;
}

static int mvcam_readl_reg(int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
{
    int err;
    uint8_t buf[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xff)};
    uint8_t bufout[8] = {0};
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msgs[2] = {
        {
            .addr = i2c_addr,
            .flags = 0,
            .len = 2,
            .buf = buf,
        },
        {
            .addr = i2c_addr,
            .flags = I2C_M_RD,
            .len = 4,
            .buf = bufout,
        },
    };

    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    err = ioctl(fd, I2C_RDWR, &msgset);
    if (err != static_cast<int>(msgset.nmsgs)) {
        return -1;
    }
    *val = ntohl(*(int32_t *) bufout);
    return 0;
}

static int mvcam_writel_reg(int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
{
    int ret = 0;

    unsigned char msg[8] = {
        static_cast<unsigned char>(reg >> 8),
        static_cast<unsigned char>(reg),
        static_cast<unsigned char>(val >> 24),
        static_cast<unsigned char>(val >> 16),
        static_cast<unsigned char>(val >> 8),
        static_cast<unsigned char>(val),
    };
    int len = 6;

    if (write(fd, msg, len) != len) {
        fprintf(stderr, "Failed to write register index %d,please try again\r\n", reg);
        ret = -1;
    } else {
        //fprintf(stderr, " write register data %d\r\n", regs[i].data);
    }

    return ret;
}

static int readl_reg(EuCamType camType, int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
{
    int ret = QX_FAILURE;
    switch (camType) {
    case EuCamType::GxCamera:
        ret = gxcam_readl_reg(fd, i2c_addr, reg, val);
        break;
    case EuCamType::MvCamera:
        ret = mvcam_readl_reg(fd, i2c_addr, reg, val);
        break;
    default:
        break;
    }
    return ret;
}

static int writel_reg(EuCamType camType, int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
{
    int ret = QX_FAILURE;
    switch (camType) {
    case EuCamType::GxCamera:
        ret = gxcam_writel_reg(fd, i2c_addr, reg, val);
        break;
    case EuCamType::MvCamera:
        ret = mvcam_writel_reg(fd, i2c_addr, reg, val);
        break;
    default:
        break;
    }
    return ret;
}

Camera::Camera(CommDevCfg devCfg, QObject *parent)
    : QObject{parent}
    , m_devCfg(devCfg)
{
    m_bThread = true;
    // 保存线程句柄，析构时 join（见 ~Camera）
    m_threadCmd = std::thread([this]() { cmdThread(); });
}

Camera::~Camera()
{
    // 先关设备：采集线程（Capture 本身就是 QThread）必须先 stopStream()+wait() 再销毁，
    // 否则 deleting a running QThread 会直接崩溃。
    CloseDevice();
    // 再置停止标志并 join：cmdThread 单次循环最长耗时为
    // 一次 i2c 操作(含锁超时) + 最多 100ms 等待，因此 join 不会长时间阻塞 GUI。
    m_bThread = false;
    if (m_threadCmd.joinable()) {
        m_threadCmd.join();
    }
}

bool Camera::OpenDevice(StCamParam &param)
{
    bool bRet = false;
    if ((-1 == m_fd) && (nullptr == m_capture)) {
        m_fd = open(param.videoNode.toStdString().c_str(), O_RDWR | O_NONBLOCK);
        if (-1 != m_fd) {
            struct v4l2_capability cap;
            int ret = ioctl(m_fd, VIDIOC_QUERYCAP, &cap);
            if (ret < 0) {
                qWarning() << "获取功能失败:" << errno;
            } else {
                if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                    m_capture = std::make_unique<CaptureSPlane>(m_devCfg, this);
                    qDebug() << "get capabilities:V4L2_CAP_VIDEO_CAPTURE";
                } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
                    m_capture = std::make_unique<CaptureMPlane>(m_devCfg, this);
                    qDebug() << "get capabilities:V4L2_CAP_VIDEO_CAPTURE_MPLANE";
                } else {
                    qWarning() << "get capabilities:unknow";
                }
            }
            if (m_capture) {
                bRet = true;
                m_capture.get()->setMainWidget(parent());
                // 多相机：show/save 不再通过 MainWidget 全局信号广播，改由 Camera 转发方法直达本通道 Capture。
                // 帧信息/状态携带通道号回传，避免多通道互相覆盖。
                MainWidget *mw = qobject_cast<MainWidget *>(parent());
                const int ch = m_devCfg.channel;
                connect(m_capture.get(),
                        &Capture::sndFrameInfo,
                        mw,
                        [mw, ch](const StCamInfo &info) { mw->rcvFrameInfo(ch, info); });
                connect(m_capture.get(),
                        &Capture::sndSaveFinish,
                        mw,
                        &MainWidget::rcvSaveFinish);
                connect(m_capture.get(),
                        &Capture::sndStatus,
                        mw,
                        [mw, ch](EuCamStatus status) { mw->onCamStatus(ch, status); });
                m_capture->startStream(m_fd, param);
                m_refCount++;
            }
        } else {
            qWarning() << "...........OpenDevice.....error.........." << m_fd;
        }
    }
    qDebug() << "...........OpenDevice.............." << m_refCount;
    return bRet;
}

void Camera::CloseDevice()
{
    if (m_capture) {
        m_capture->stopStream();
        m_capture->disconnect();
        // 以前用 release() 丢弃指针：Capture 作为本对象的 QObject 子对象会一直累积，
        // 反复“开始/停止采集”会泄漏整个 Capture（及其 QTimer、已结束的线程对象）。
        // reset() 删除对象，~QObject 会自动把自己从父对象子列表里摘掉，不会重复释放。
        m_capture.reset();
        m_refCount--;
    }
    if (-1 != m_fd) {
        close(m_fd);
        m_fd = -1;
    }
}

void Camera::setShow(bool bShow)
{
    // Capture 对象与 MainWidget 同属 GUI 线程，直接调用等价于原 Queued 信号
    if (m_capture) {
        m_capture->rcvShow(bShow);
    }
}

void Camera::saveImg(QString strPath, int count)
{
    if (m_capture) {
        m_capture->rcvSaveImg(strPath, count);
    }
}

void Camera::runSystemCmd(QString strCmd)
{
    // 以前这条路径是 system() + /bin/sh + 固定名临时文件 + sprintf 拼接到 256 字节栈缓冲区，
    // 命令里携带的 ROI/分辨率/设备名均来自界面输入或相机上报的拓扑 JSON，存在注入与栈溢出风险。
    // 现在自己分词后用 QProcess 直接执行，不经过 shell，也不再把输出重定向到临时文件。
    const QStringList argv = QxHelp::splitCmdLine(strCmd);
    if (argv.size() < 2) {
        emit sndOptLog(-1, strCmd);
        return;
    }

    int ret = -1;
    QString output;
    m_mtxCmd.lock();
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(argv.first(), argv.mid(1));
    if (!process.waitForStarted(3000)) {
        output = QStringLiteral("start failed: ") + argv.first();
        ret = -1;
    } else if (!process.waitForFinished(20000)) {
        process.kill();
        process.waitForFinished(1000);
        output = QStringLiteral("timeout: ") + strCmd;
        ret = -2;
    } else {
        output = QString::fromLocal8Bit(process.readAll()).trimmed();
        ret = process.exitCode();
    }
    m_mtxCmd.unlock();

    emit sndOptLog(ret, strCmd);
    if (0 != ret && !output.isEmpty()) {
        emit sndOptLog(ret, output);
    }
}

void Camera::asynReadData(StI2CCmd cmd)
{
    m_mtx.lock();
    m_listCmd.push_back(cmd);
    m_mtx.unlock();
}

void Camera::asynWriteDate(StI2CCmd cmd)
{
    m_mtx.lock();
    m_listCmd.push_back(cmd);
    m_mtx.unlock();
}

void Camera::syncReadData(StI2CCmd &cmd)
{
    m_mtx.lock();
    readData(cmd);
    m_mtx.unlock();
}

int Camera::readData(StI2CCmd &cmd)
{
    int ret = QX_FAILURE;
    QString strMsg;
    QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
    
    // 使用I2C进程锁保护设备访问
    I2CLockGuard lockGuard(cmd.i2c.toStdString());
    if (!lockGuard.isLocked()) {
        strMsg = QString("i2c_read failed to acquire lock for %1").arg(cmd.i2c);
        emit sndOptLog(-1, strMsg, cmd.showlog);
        return QX_FAILURE;
    }
    
    int fd = open(cmd.i2c.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        if (0 == ioctl(fd, I2C_SLAVE_FORCE, cmd.addr)) {
            if (QX_SUCCESS == readl_reg(cmd.camType, fd, cmd.addr, cmd.reg, &cmd.data)) {
                QString strData = QString("0x%1").arg(cmd.data, 4, 16, QLatin1Char('0'));
                strMsg = QString("i2c_read regAddr:%1\tdata:[hex]:%2--[dec]:%3")
                             .arg(strReg, strData)
                             .arg(cmd.data);
                emit sndOptLog(0, strMsg, cmd.showlog);
            } else {
                strMsg = QString("i2c_read regAddr %1 failed").arg(strReg);
                emit sndOptLog(-1, strMsg, cmd.showlog);
            }
            ret = QX_SUCCESS;
        } else {
            strMsg = QString("i2c_read I2C_SLAVE_FORCE %1 %2").arg(cmd.i2c, strerror(errno));
            emit sndOptLog(errno, strMsg, cmd.showlog);
        }
        close(fd);
    } else {
        strMsg = QString("i2c_read open %1 %2").arg(cmd.i2c, strerror(errno));
        emit sndOptLog(fd, strMsg, cmd.showlog);
    }
    if (cmd.showlog) {
        qDebug() << strMsg;
    }
    return ret;
}

int Camera::writeData(StI2CCmd &cmd)
{
    int ret = -1;
    QString strMsg;
    QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
    QString strData = QString("0x%1").arg(cmd.data, 4, 16, QLatin1Char('0'));
    
    // 使用I2C进程锁保护设备访问
    I2CLockGuard lockGuard(cmd.i2c.toStdString());
    if (!lockGuard.isLocked()) {
        strMsg = QString("i2c_write failed to acquire lock for %1").arg(cmd.i2c);
        emit sndOptLog(-1, strMsg, cmd.showlog);
        return -1;
    }
    
    int fd = open(cmd.i2c.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        if (ioctl(fd, I2C_SLAVE_FORCE, cmd.addr) < 0) {
            strMsg = QString("i2c_wirte I2C_SLAVE_FORCE %1 %2 %3")
                         .arg(cmd.i2c, strReg, strerror(errno));
            emit sndOptLog(errno, strMsg, cmd.showlog);
            close(fd);
            return ret;
        }
        if (QX_SUCCESS == writel_reg(cmd.camType, fd, cmd.addr, cmd.reg, cmd.data)) {
            ret = 0;
            strMsg = QString("i2c_wirte regAddr:%1\tdata:[hex]:%2--[dec]:%3")
                         .arg(strReg, strData)
                         .arg(cmd.data);
            emit sndOptLog(0, strMsg, cmd.showlog);
        } else {
            strMsg = QString("i2c_wirte regAddr %1 failed").arg(strReg);
            emit sndOptLog(-1, strMsg, cmd.showlog);
        }
        close(fd);

    } else {
        strMsg = QString("i2c_wirte open %1 %2").arg(cmd.i2c, strerror(errno));
        emit sndOptLog(fd, strMsg, cmd.showlog);
    }
    if (cmd.showlog) {
        qDebug() << strMsg;
    }
    return ret;
}

void Camera::cmdSleep(int n, bool bReal)
{
    if (bReal) {
        QThread::msleep(n);
    }
    emit sndOptLog(0, QString("sleep %1 ms").arg(n), true);
}

void Camera::cmdThread()
{
    while (m_bThread) {
        int count = 0;
        StI2CCmd cmd;
        {
            m_mtx.lock();
            count = m_listCmd.size();
            if (count > 0) {
                cmd = m_listCmd.front();
                m_listCmd.pop_front();
            }
            m_mtx.unlock();
        }
        if (count > 0) {
            if (cmd.bWrite) {
                m_mtxCmd.lock();
                writeData(cmd);
                m_mtxCmd.unlock();
                if (cmd.cmd == def_cmd) {
                    cmdSleep(10);
                }
                if (cmd.cmd == trgmode) {
                    QThread::msleep(100);
                }
            } else {
                QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
                m_mtxCmd.lock();
                readData(cmd);
                m_mtxCmd.unlock();
                StCmdRet ret;
                ret.cmd = cmd.cmd;
                ret.data = cmd.data;
                ret.obj = cmd.obj;
                ret.show = cmd.showlog;
                ret.strReg = strReg;
                emit sndCmdRet(ret);
            }
        }
        QThread::msleep(1);
    }
}
