#include "capturesplane.h"
#include "mainwidget.h"

CaptureSPlane::CaptureSPlane(QObject *parent)
    : Capture{parent}
{}

void CaptureSPlane::startStream(int fd, StCamParam &param)
{
    Capture::startStream(fd, param);
    m_bRunning = true;
    start();
}

void CaptureSPlane::stopStream()
{
    Capture::stopStream();
    m_bRunning = false;
    wait();
}

void CaptureSPlane::initBuf()
{
    qDebug() << "...initBuf...." << m_showFPS;

    m_v4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_G_FMT:");
    }

    //qDebug() << "...get.1..." << m_v4l2Fmt.fmt.pix_mp.width << m_v4l2Fmt.fmt.pix_mp.height;

    m_v4l2Fmt.fmt.pix.width = m_stCamParam.w;
    m_v4l2Fmt.fmt.pix.height = m_stCamParam.h;
    m_v4l2Fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    m_v4l2Fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(m_fd, VIDIOC_S_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_S_FMT:");
    }

    if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_G_FMT:");
    }

    qDebug() << "...get..." << m_v4l2Fmt.fmt.pix.width << m_v4l2Fmt.fmt.pix.height;

    m_v4l2ReqBuf.count = VIDEO_BUFFER_COUNT; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf) == -1) {
        perror("ERROR VIDIOC_REQBUFS:");
    }

    //qDebug() << "...m_v4l2ReqBuf.count...." << m_v4l2ReqBuf.count;
    m_buffers = (StSplaneBuffer *) calloc(m_v4l2ReqBuf.count, sizeof(StSplaneBuffer));
    for (int i = 0; i < m_v4l2ReqBuf.count; ++i) {
        memset(&m_v4l2Buf, 0, sizeof(m_v4l2Buf));
        m_v4l2Buf.index = i;
        m_v4l2Buf.memory = V4L2_MEMORY_MMAP;
        m_v4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //从内核空间查询一空间做映射
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QUERYBUF:");
        }

        m_buffers[i].length = m_v4l2Buf.length;
        m_buffers[i].start = mmap(nullptr,
                                  m_v4l2Buf.length,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  m_fd,
                                  m_v4l2Buf.m.offset);
        if (MAP_FAILED == m_buffers[i].start) {
            perror("MAP_FAILED == buffers[i].start...........");
        }

        if (ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QBUF:");
        }
    }

    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &bufType) == -1) {
        perror("ERROR VIDIOC_STREAMON:");
    }
}

void CaptureSPlane::freeBuf()
{
    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &bufType) == -1) {
        perror("VIDIOC_STREAMOFF");
    }

    for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
        m_v4l2Buf.index = i;
        ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf);
        munmap(m_buffers[i].start, m_buffers[i].length);
    }

    free(m_buffers);
    m_buffers = nullptr;

    memset(&m_v4l2ReqBuf, 0, sizeof(m_v4l2ReqBuf));
    m_v4l2ReqBuf.count = 0; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (0 != ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf)) {
        perror("ERROR: failed to VIDIOC_REQBUFS");
    }
    qDebug() << "...freeBuf....";
}

void CaptureSPlane::run()
{
    qDebug() << "...............run.....................enter";
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    initBuf();
    int nBytesperline = 0;
    unsigned char *pBuf = nullptr;
    int strategy = new_buf;
    fd_set readfds;
    timeval timeout;
    //QElapsedTimer timer;
    QElapsedTimer timerFps;
    struct timeval ts;
    emit sndStatus(1);
    bool bNeedSndShow = false;
    timerFps.start();
    int seconds = 0;
    int nFps = 0;

    qint64 lastFrameTS = 0;
    while (m_bRunning) {
        FD_ZERO(&readfds);
        FD_SET(m_fd, &readfds);

        double m_frameInterval = 1000.0 / m_showFPS;

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int nfds = m_fd + 1;
        int ready = select(nfds, &readfds, nullptr, nullptr, &timeout);
        if (-1 == ready) {
            qDebug() << "read data error!" << errno;
        } else if (0 == ready) {
            //qDebug() << "read data time out!"
        } else {
            if (FD_ISSET(m_fd, &readfds)) {
                while (0 == ioctl(m_fd, VIDIOC_DQBUF, &m_v4l2Buf)) {
                    //从缓冲队列获取一帧数据（出队列）
                    //qDebug() << "....VIDIOC_DQBUF....." << m_v4l2Buf.index;
                    //for (int j = 0; j < num_planes; j++) {
                    //qDebug() << ".....num_planes.." << num_planes;
                    nBytesperline = m_v4l2Fmt.fmt.pix.bytesperline;
                    m_width = m_v4l2Fmt.fmt.pix_mp.width;
                    m_height = m_v4l2Fmt.fmt.pix_mp.height;
                    pBuf = (unsigned char *) m_buffers[m_v4l2Buf.index].start;
                    int len = m_buffers[m_v4l2Buf.index].length;
                    ts = m_v4l2Buf.timestamp;
                    QString strTS = QString::asprintf("%ld.%06ld", ts.tv_sec, ts.tv_usec);
                    qint64 fpsTime = timerFps.elapsed();
                    nFps++;
                    if (fpsTime > 999) {
                        timerFps.restart();
                        seconds++;
                        StCamInfo info;
                        info.fps = nFps * 1.0 / seconds;
                        info.height = m_height;
                        info.width = m_width;
                        info.left = m_x;
                        info.top = m_y;
                        emit sndFrameInfo(info);
                    }

                    bool shouldDisplay = false;
                    qint64 curFrameTS = ts.tv_sec * 1000 + ts.tv_usec / 1000;
                    if (curFrameTS - lastFrameTS > m_frameInterval) {
                        lastFrameTS = curFrameTS;
                        shouldDisplay = true;
                    }

                    if (m_bShow) {
                        if (shouldDisplay) {
                            StImgInfo sii;
                            //timer.restart(); // 开始计时
                            if (new_buf == strategy) {
                                sii.buf = new unsigned char[len];
                                memcpy(sii.buf, pBuf, len);
                            } else if (v4l2_buf == strategy) {
                                sii.buf = pBuf;
                            } else if (change_img == strategy) {
                                sii.img = QImage(pBuf,
                                                 nBytesperline,
                                                 m_height,
                                                 QImage::Format_Grayscale8);
                            }

                            sii.strategy = strategy;
                            sii.len = len;
                            sii.fps = nFps;
                            sii.width = nBytesperline;
                            sii.height = m_height;
                            sii.strTS = strTS;
                            //emit sndCaptureImg(sii);
                            if (m_mainWidget) {
                                MainWidget *pMain = qobject_cast<MainWidget *>(m_mainWidget);
                                if (pMain) {
                                    pMain->pushData(sii);
                                }
                            }
                        }
                        //qint64 elapsedTime = timer.elapsed(); // 获取经过的时间，单位为微秒
                        // if (elapsedTime > TIEM_OUT) {
                        //     qDebug() << "read data:" << elapsedTime << "microseconds";
                        // }
                        bNeedSndShow = true;
                    } else {
                        if (bNeedSndShow) {
                            bNeedSndShow = false;
                            emit sndStatus(2);
                        }
                    }
                    //save data push list
                    if (m_bSave && m_nSaveCount > 0) {
                        QString strIndex = strTS + "_" + QString::number(++m_saveIndex);
                        QString strTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
                        QString strFilename = m_strSavePah
                                              + QString("%1_%2x%3_%4.RAW")
                                                    .arg(strTime)
                                                    .arg(nBytesperline)
                                                    .arg(m_height)
                                                    .arg(strIndex);
                        m_mtxSaveImg.lock();
                        StSaveImg si;
                        //if (len < si.buf_size) {
                        si.buf = new unsigned char[len];
                        memcpy(si.buf, pBuf, len);
                        si.len = len;
                        si.width = nBytesperline;
                        si.height = m_height;
                        si.path = m_strSavePah;
                        si.filename = strFilename;
                        m_listSaveImg.push_back(si);
                        m_nSaveCount--;
                        // }
                        m_mtxSaveImg.unlock();
                    }
                    if (m_nSaveCount <= 0) {
                        m_bSave = false;
                    }
                    //}
                    //读取数据后将缓冲区放入队列
                    //qDebug() << "....VIDIOC_QBUF....." << m_v4l2Buf.index;
                    if (0 != ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf)) {
                        perror("ERROR: VIDIOC_QBUF ");
                        break;
                    }
                }
                // //读取数据后将缓冲区放入队列
                // if (0 != ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf)) {
                //     perror("ERROR: VIDIOC_QBUF ");
                //     break;
                // }
                // 检查是否是因为队列为空而退出
                // if (errno != EAGAIN) {
                //     qDebug() << "Error during buffer clearing.";
                // } else {
                //     qDebug() << "Buffer queue cleared successfully.";
                // }
            }
        }
        msleep(1);
    }
    freeBuf();
    emit sndStatus(0);
    //QTimer::singleShot(300, this, [this]() {  });
    qDebug() << "...............run.....................leave";
}
