#include "capturemplane.h"
#include "mainwidget.h"

CaptureMPlane::CaptureMPlane(QObject *parent)
    : Capture{parent}
{

}

CaptureMPlane::~CaptureMPlane() {}

void CaptureMPlane::startStream(int fd, StCamParam &param)
{
    Capture::startStream(fd, param);
    m_bRunning = true;
    start();
}

void CaptureMPlane::stopStream()
{
    Capture::stopStream();
    m_bRunning = false;
    wait();
}

void CaptureMPlane::initBuf()
{
    qDebug() << "...initBuf...." << m_showFPS;
    // m_v4l2CropCap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // if (-1 == ioctl(m_fd, VIDIOC_CROPCAP, &m_v4l2CropCap) && errno != EINVAL) {
    //     perror("ERROR VIDIOC_CROPCAP:");
    // } else {
    //     printf("Bounds: Left %d, Top %d, Width %d, Height %d\n",
    //            m_v4l2CropCap.bounds.left,
    //            m_v4l2CropCap.bounds.top,
    //            m_v4l2CropCap.bounds.width,
    //            m_v4l2CropCap.bounds.height);
    //     printf("DefRect: Left %d, Top %d, Width %d, Height %d\n",
    //            m_v4l2CropCap.defrect.left,
    //            m_v4l2CropCap.defrect.top,
    //            m_v4l2CropCap.defrect.width,
    //            m_v4l2CropCap.defrect.height);

    //     printf("Pixelaspect: denominator %d, numerator %d\n",
    //            m_v4l2CropCap.pixelaspect.denominator,
    //            m_v4l2CropCap.pixelaspect.numerator);
    // }
    // m_v4l2Crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // m_v4l2Crop.c.left = m_stCamParam.x;
    // m_v4l2Crop.c.top = m_stCamParam.y;
    // m_v4l2Crop.c.width = m_stCamParam.w;
    // m_v4l2Crop.c.height = m_stCamParam.h;
    // if (-1 == ioctl(m_fd, VIDIOC_S_CROP, &m_v4l2Crop)) {
    //     perror("ERROR VIDIOC_S_CROP:");
    // }
    // m_v4l2Crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // if (-1 == ioctl(m_fd, VIDIOC_G_CROP, &m_v4l2Crop)) {
    //     perror("ERROR VIDIOC_G_CROP:");
    // }
    // qDebug() << "...crop...." << m_v4l2Crop.c.left << m_v4l2Crop.c.top << m_v4l2Crop.c.width
    //          << m_v4l2Crop.c.height;

    m_v4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_G_FMT:");
    }

    //qDebug() << "...get.1..." << m_v4l2Fmt.fmt.pix_mp.width << m_v4l2Fmt.fmt.pix_mp.height;

    m_v4l2Fmt.fmt.pix_mp.width = m_stCamParam.w;
    m_v4l2Fmt.fmt.pix_mp.height = m_stCamParam.h;

    //qDebug() << "...set...." << m_v4l2Fmt.fmt.pix_mp.width << m_v4l2Fmt.fmt.pix_mp.height;
    if (ioctl(m_fd, VIDIOC_S_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_S_FMT:");
    }

    // if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
    //     perror("ERROR VIDIOC_G_FMT:");
    // }

    //qDebug() << "...get.2..." << m_v4l2Fmt.fmt.pix_mp.width << m_v4l2Fmt.fmt.pix_mp.height;

    m_v4l2ReqBuf.count = VIDEO_BUFFER_COUNT; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf) == -1) {
        perror("ERROR VIDIOC_REQBUFS:");   
    }

    //qDebug() << "...m_v4l2ReqBuf.count...." << m_v4l2ReqBuf.count;
    num_planes = m_v4l2Fmt.fmt.pix_mp.num_planes;
    m_buffers = (StMplaneBuffer *) calloc(m_v4l2ReqBuf.count, sizeof(StMplaneBuffer));
    for (int i = 0; i < m_v4l2ReqBuf.count; ++i) {
        v4l2_plane *planes = (v4l2_plane *) calloc(num_planes, sizeof(v4l2_plane));
        StMmapAddr *addrs = (StMmapAddr *) calloc(num_planes, sizeof(StMmapAddr));

        m_v4l2Buf.index = i;
        m_v4l2Buf.memory = V4L2_MEMORY_MMAP;
        m_v4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        m_v4l2Buf.m.planes = planes;
        m_v4l2Buf.length = num_planes;
        //从内核空间查询一空间做映射
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QUERYBUF:");
        }

        m_buffers[i].planes = planes;
        m_buffers[i].addrs = addrs;

        for (int j = 0; j < num_planes; ++j) {
            m_buffers[i].addrs[j].start = mmap(nullptr,
                                               m_v4l2Buf.m.planes[j].length,
                                               PROT_READ | PROT_WRITE,
                                               MAP_SHARED,
                                               m_fd,
                                               m_v4l2Buf.m.planes[j].m.mem_offset);
            if (MAP_FAILED == m_buffers[i].addrs[j].start) {
                perror("MAP_FAILED == buffers[i].plane_start[j].start...........");
            }
        }
        if (ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QBUF:");
        }
    }

    //for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &bufType) == -1) {
        perror("ERROR VIDIOC_STREAMON:");
    }
    //}
}

void CaptureMPlane::freeBuf()
{
    //for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &bufType) == -1) {
        perror("VIDIOC_STREAMOFF");
    }
    //}

    for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
        m_v4l2Buf.index = i;
        ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf);
        for (int j = 0; j < num_planes; ++j) {
            munmap(m_buffers[i].addrs[j].start, m_buffers[i].planes[j].length);
        }
    }

    for (int i = 0; i < m_v4l2ReqBuf.count; ++i) {
        free(m_buffers[i].planes);
        m_buffers[i].planes = nullptr;
        free(m_buffers[i].addrs);
        m_buffers[i].addrs = nullptr;
    }
    free(m_buffers);
    m_buffers = nullptr;

    memset(&m_v4l2ReqBuf, 0, sizeof(m_v4l2ReqBuf));
    m_v4l2ReqBuf.count = 0; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (0 != ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf)) {
        perror("ERROR: failed to VIDIOC_REQBUFS");
    }
    qDebug() << "...freeBuf....";
}

void CaptureMPlane::run()
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
    double m_frameCounter; // 浮点型计数器，用于非整数倍丢帧
    double m_dropInterval; // 每隔多少帧保留一帧

    while (m_bRunning) {
        FD_ZERO(&readfds);
        FD_SET(m_fd, &readfds);

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
                    for (int j = 0; j < num_planes; j++) {
                        //qDebug() << ".....num_planes.." << num_planes;
                        nBytesperline = m_v4l2Fmt.fmt.pix_mp.plane_fmt[j].bytesperline;
                        m_width = m_v4l2Fmt.fmt.pix_mp.width;
                        m_height = m_v4l2Fmt.fmt.pix_mp.height;
                        pBuf = (unsigned char *) m_buffers[m_v4l2Buf.index].addrs[j].start;
                        int len = m_buffers[m_v4l2Buf.index].planes[j].length;
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

                        bool shouldDisplay = true;

                        m_dropInterval = m_stCamParam.fps * 1.0 / m_showFPS;

                        m_frameCounter += 1.0;
                        if (m_frameCounter >= m_dropInterval) {
                            m_frameCounter -= m_dropInterval;

                            shouldDisplay = true;
                        } else {
                            shouldDisplay = false;
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
                            QString strTime = QDateTime::currentDateTime().toString(
                                "yyyyMMddHHmmss");
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
                    }
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
