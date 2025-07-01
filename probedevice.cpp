#include "probedevice.h"
#include "commheader.h"
#include <fstream>

ProbeDevice::ProbeDevice(QObject *parent)
    : QObject{parent}
{
    m_timer = new QTimer(this);
}

void ProbeDevice::Start()
{
    connect(m_timer, &QTimer::timeout, this, [this]() {

    });
    m_timer->start(5000);

    QTimer::singleShot(1000, this, [this]() {

        //listVideo();
    });
}

void ProbeDevice::Stop()
{
    if (m_timer) {
        m_timer->stop();
    }
}

bool ProbeDevice::TestDev(QString &deviceName, int &nBufType)
{
    bool bRet = false;
    int fd = open(deviceName.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        struct v4l2_capability cap;
        int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);

        if (ret < 0) {
            perror("获取功能失败");
        } else {
            qDebug() << "...capabilities..ok..." << ret;
            if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                bRet = true;
            } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
                nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                bRet = true;
            } else {
                qDebug() << "capabilities........no...........CAPTURE";
            }

            if (bRet) {
                listFmt(fd, nBufType);
            }
        }
        close(fd);
    }
    return bRet;
}

// EuFW_Type ProbeDevice::fwType()
// {
//     EuFW_Type fw = FW_DEF;
//     std::ifstream modelFile("/sys/firmware/devicetree/base/model");
//     std::string model;
//     if (modelFile) {
//         std::getline(modelFile, model);
//         //qDebug() << "/sys/firmware/devicetree/base/model:" << QString::fromStdString(model);
//         if (model.find("CM5") != std::string::npos) {
//             fw = RK_CM5;
//         } else if (model.find("CM4") != std::string::npos) {
//             fw = RK_CM4;
//         }
//     }
//     return fw;
// }

void ProbeDevice::listVideo()
{
    int deviceCount = 0;
    do {
        QString strDev = QString("/dev/video%1").arg(deviceCount);
        //QString strDev = QString("/dev/i2c-%1").arg(deviceCount);
        int fd = open(strDev.toStdString().c_str(), O_RDWR);
        if (-1 != fd) {
            struct v4l2_capability cap;
            int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret < 0) {
                perror("获取功能失败");
            } else {
                // qDebug() << QString::asprintf("DriverName:%s--Card Name:%s--Bus "
                //                               "info:%s--DriverVersion:%u.%u.%u--device_caps:%d",
                //                               cap.driver,
                //                               cap.card,
                //                               cap.bus_info,
                //                               (cap.version >> 16) & 0xFF,
                //                               (cap.version >> 8) & 0xFF,
                //                               (cap.version) & 0xFF,
                //                               cap.device_caps);
                emit discovery(strDev);
            }
            close(fd);
        } else {
            //perror((strDev.toStdString() + " 失败").c_str());
            emit operateLog(fd, strDev + " " + strerror(errno));
        }
    } while (++deviceCount < 50);
}

void ProbeDevice::listFmt(int fd, int nBufType)
{
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = nBufType;
    while (0 == ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        // qDebug() << "index:" << fmtdesc.index;
        // qDebug() << "flags:" << fmtdesc.flags;
        //qDebug() << "descrrption:" << fmtdesc.description;
        qDebug() << QString::asprintf("..................pixelformat:%c%c%c%c..................",
                                      fmtdesc.pixelformat & 0XFF,
                                      (fmtdesc.pixelformat >> 8) & 0xFF,
                                      (fmtdesc.pixelformat >> 16) & 0xFF,
                                      (fmtdesc.pixelformat >> 24) & 0xFF);
        //qDebug() << "mbus_code:" << fmtdesc.mbus_code;
        listFrameSizes(fd, fmtdesc.pixelformat, nBufType);
        fmtdesc.index++;
    }
    qDebug() << "listFmt................leave";
}

void ProbeDevice::listFrameSizes(int fd, uint32_t pixelformat, int nBufType)
{
    struct v4l2_frmsizeenum fsize = {0};
    fsize.pixel_format = pixelformat;
    fsize.index = 0;
    while (0 == ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) {
        if (V4L2_FRMSIZE_TYPE_DISCRETE == fsize.type) {
            qDebug() << QString::asprintf("FRMSIZE discrete: width = %u, height = %u }",
                                          fsize.discrete.width,
                                          fsize.discrete.height);
            listFrameIntervals(fd, fsize.pixel_format, fsize.discrete.width, fsize.discrete.height);
        } else if (V4L2_FRMSIZE_TYPE_CONTINUOUS == fsize.type) {
            qDebug() << QString::asprintf("FRMSIZE continuous: {min { width = %u, height = %u }.."
                                          "max { width = %u, height = %u } }",
                                          fsize.stepwise.min_width,
                                          fsize.stepwise.min_height,
                                          fsize.stepwise.max_width,
                                          fsize.stepwise.max_height);
            break;
        } else if (V4L2_FRMSIZE_TYPE_STEPWISE == fsize.type) {
            qDebug() << QString::asprintf("FRMSIZE stepwise:{ min { width = %u, height = %u }.."
                                          "max { width = %u, height = %u }.."
                                          "stepsize { width = %u, height = %u } }",
                                          fsize.stepwise.min_width,
                                          fsize.stepwise.min_height,
                                          fsize.stepwise.max_width,
                                          fsize.stepwise.max_height,
                                          fsize.stepwise.step_width,
                                          fsize.stepwise.step_height);
            break;
        }
        fsize.index++;
    }
}

void ProbeDevice::listFrameIntervals(int fd, uint32_t pixfmt, uint32_t width, uint32_t height)
{
    struct v4l2_frmivalenum fival = {0};
    fival.index = 0;
    fival.pixel_format = pixfmt;
    fival.width = width;
    fival.height = height;
    //frmival.type = nBufType;
    while (0 == ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) {
        if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            qDebug() << "FRMIVAL discrete:" << fival.width << fival.height
                     << fival.discrete.denominator << fival.discrete.numerator;
        } else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
            qDebug() << QString::asprintf("FRMIVAL continuous {min { %u/%u }..max { %u/%u } }, ",
                                          fival.stepwise.min.numerator,
                                          fival.stepwise.min.numerator,
                                          fival.stepwise.max.denominator,
                                          fival.stepwise.max.denominator);
        } else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
            qDebug() << QString::asprintf("FRMIVAL stepwise {min { %u/%u }..max { %u/%u }.."
                                          "stepsize { %u/%u } }",
                                          fival.stepwise.min.numerator,
                                          fival.stepwise.min.denominator,
                                          fival.stepwise.max.numerator,
                                          fival.stepwise.max.denominator,
                                          fival.stepwise.step.numerator,
                                          fival.stepwise.step.denominator);
            break;
        }

        fival.index++;
    }
}
