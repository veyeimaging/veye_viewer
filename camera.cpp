#include "camera.h"
#include "capturemplane.h"
#include "capturesplane.h"
#include "mainwidget.h"

Camera::Camera(QObject *parent)
    : QObject{parent}
{
}

Camera::~Camera() {}

bool Camera::OpenDevice(StCamParam &param)
{
    bool bRet = false;
    m_fd = open(param.videoNode.toStdString().c_str(), O_RDWR | O_NONBLOCK);
    if (-1 != m_fd) {
        struct v4l2_capability cap;
        int ret = ioctl(m_fd, VIDIOC_QUERYCAP, &cap);
        if (ret < 0) {
            qWarning() << "获取功能失败:" << errno;
        } else {
            if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                m_capture = std::make_unique<CaptureSPlane>(this);
                qDebug() << "get capabilities: V4L2_CAP_VIDEO_CAPTURE";
            } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
                m_capture = std::make_unique<CaptureMPlane>(this);
                qDebug() << "get capabilities: V4L2_CAP_VIDEO_CAPTURE_MPLANE";
            } else {
                qWarning() << "get capabilities:unknow";
            }
        }

        if (m_capture) {
            bRet = true;
            m_capture.get()->setMainWidget(parent());
            connect((MainWidget *) parent(),
                    &MainWidget::sndSaveImg,
                    m_capture.get(),
                    &Capture::rcvSaveImg);
            connect((MainWidget *) parent(),
                    &MainWidget::sndShow,
                    m_capture.get(),
                    &Capture::rcvShow);
            connect((MainWidget *) parent(),
                    &MainWidget::sndShowFps,
                    m_capture.get(),
                    &Capture::rcvShowFps);
            connect(m_capture.get(),
                    &Capture::sndCaptureImg,
                    qobject_cast<MainWidget *>(parent()),
                    &MainWidget::rcvCaptureImg,
                    Qt::QueuedConnection);
            connect(m_capture.get(),
                    &Capture::sndFrameInfo,
                    qobject_cast<MainWidget *>(parent()),
                    &MainWidget::rcvFrameInfo);
            connect(m_capture.get(),
                    &Capture::sndSaveFinish,
                    qobject_cast<MainWidget *>(parent()),
                    &MainWidget::rcvSaveFinish);
            connect(m_capture.get(),
                    &Capture::sndStatus,
                    qobject_cast<MainWidget *>(parent()),
                    &MainWidget::rcvStatus);
            m_capture->startStream(m_fd, param);
            qDebug() << "...........m_capture->startStream.........." << m_fd;
        }
    } else {
        qDebug() << "...........OpenDevice.....error.........." << m_fd;
    }
    return bRet;
}

void Camera::CloseDevice()
{
    if (m_capture) {
        m_capture->stopStream();
        m_capture->disconnect();
        m_capture.release();
    }
    if (-1 != m_fd) {
        close(m_fd);
        m_fd = -1;
    }
}
